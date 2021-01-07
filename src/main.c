/*
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 ******************************************************************************/



#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <dbus/dbus.h>
#include "sts_queue.h"
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "screendriver.h"

static DBusHandlerResult draw_messages(DBusConnection *connection, DBusMessage *message, void *user_data);
static void check_and_abort(DBusError *error);
void Handler(int signo);
static void respond_to_introspect(DBusConnection *connection, DBusMessage *request);
static void respond_to_apply(DBusConnection *connection, DBusMessage *request);


int SERV_RUNNING = 1;
pthread_t thread_executor;
StsHeader *command_buffer;

int queue_nonempty = 0;
pthread_mutex_t mx_queue_nonempty;	// Controls access to the queue
pthread_cond_t cv_queue_nonempty;	// Signals that the queue is nonempty


// ============================================================================
// ==	THREADS
//


// It is the responsibility of the excecutor to safely free all memory used by the commands it excecutes
struct api_command {
	void (*command) (void*);		// Pointer to a (screendriver) command
	void* args;						// Struct of the arguments
};



/*
 * executor thread main function
 *
 *	 to cleanly shutdown thread, toggle RUN_EXEC flag to false and wake 
 *
 *	 responsible for handling deallocation of memory
 *
 */

int RUN_EXEC = 1;
void exec() {
	struct api_command *mem = NULL;
	
	while(RUN_EXEC) {
		pthread_mutex_lock(&mx_queue_nonempty);		// Lock mutex
		mem = StsQueue.pop(command_buffer);			// Pop the next command
		if (mem == NULL){ 							// If the queue is empty...
			printf("[server:executor] Queue empty, going to sleep... \n");
			queue_nonempty = 0;						// Let everyone know the queue is empty
			while (!queue_nonempty) {				// Ensure queue is still nonempty
				pthread_cond_wait(&cv_queue_nonempty, &mx_queue_nonempty);	// Sleep until signalled nonempty
				if (!RUN_EXEC) return;
			}
			printf("[server:executor] Queue nonempty, waking...\n");
			mem = StsQueue.pop(command_buffer);			// Pop the next command 
		}
		pthread_mutex_unlock(&mx_queue_nonempty);	// Allow access to queue again
		
		// Excecute the command		
		void (*c) (void*) = mem->command;
		c(mem->args);


		// Free memory
		if (mem->args != NULL)
				free(mem->args);
		free(mem);
		mem = NULL;
	}
}




void Handler(int signo)
{
    // System Exit
    printf("\r\n[server] exit signal recieved, attepting clean shutdown\r\n");
	exit_clean();
}

void exit_clean() {
	RUN_EXEC = 0;
	pthread_cond_signal(&cv_queue_nonempty);
	if(pthread_join(thread_executor, NULL)) {
		printf("[server] Error joining thread\n");
		exit(2);
	}

	pthread_mutex_destroy(&mx_queue_nonempty);
	StsQueue.destroy(command_buffer);
	pthread_cond_destroy(&cv_queue_nonempty);
	
	d_doclose(NULL);

	exit(0);
}


// ============================================================================
// ==	DBUS
//

static void check_and_abort(DBusError *error) {
	if (dbus_error_is_set(error)) {
		printf(error->message);
		getchar();
		abort();
	}
}



static void EnqCommand(void (*command) (void), void* args) {
	// Allocate space for API command
	struct api_command *c = malloc(sizeof(struct api_command));
	c->args = args;
	c->command = command;
	
	// If excecution gets here, enqueue the command we interpreted
	pthread_mutex_lock(&mx_queue_nonempty);		// Lock the mutex (nobody can access the queue)
	StsQueue.push(command_buffer, c);			// Push element to queue
	queue_nonempty = 1;
	pthread_cond_signal(&cv_queue_nonempty);	// Signal that the queue is nonempty
	pthread_mutex_unlock(&mx_queue_nonempty);	// Unlock the mutex, exceution can begin
}


static void reply_result(DBusConnection *connection, DBusMessage *request, int success) {
	DBusMessage *reply;
    reply = dbus_message_new_method_return(request);
	
	dbus_message_append_args(reply, DBUS_TYPE_BOOLEAN, &success, DBUS_TYPE_INVALID);
	dbus_connection_send(connection, reply, NULL);
	dbus_message_unref(reply);
}




static DBusHandlerResult draw_messages(DBusConnection *connection, DBusMessage *message, void *user_data) {
	// Recieve the message	
	const char *interface_name = dbus_message_get_interface(message);
	const char *member_name = dbus_message_get_member(message);
	printf("[server] command recieved, \tI: %s, \tM:%s\n", interface_name, member_name);
	
	
	// Introspect
	if (0==strcmp("org.freedesktop.DBus.Introspectable", interface_name) 
					&& 0==strcmp("Introspect", member_name)) {
			respond_to_introspect(connection, message);
			return DBUS_HANDLER_RESULT_HANDLED;
	}

	// Server close
	else if (0==strcmp("io.markusde.epaper", interface_name) && 0==strcmp("serv_close", member_name)) {
		printf("\tClosing server...\n");
		SERV_RUNNING = 0;
		reply_result(connection, message, 1);
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	

	if (0==strcmp("io.markusde.epaper", interface_name) && 0==strcmp("setup", member_name)) { 
		printf("[server] Enqueuing setup...\n");	
		EnqCommand(d_dosetup, NULL);
		reply_result(connection, message, 1);
	}
	else if (0==strcmp("io.markusde.epaper", interface_name) && 0==strcmp("close", member_name)) {
		printf("[server] Enqueuing close...\n");	
		EnqCommand(d_doclose, NULL);
		reply_result(connection, message, 1);
	}
	else if (0==strcmp("io.markusde.epaper", interface_name) && 0==strcmp("push", member_name)) { 
		printf("[server] Enqueuing push...\n");	
		EnqCommand(d_dopush, NULL);
		reply_result(connection, message, 1);
	}
	else if (0==strcmp("io.markusde.epaper", interface_name) && 0==strcmp("clear", member_name)) {
		printf("[server] Enqueuing clear...\n");	
		EnqCommand(d_doclear, NULL);
		reply_result(connection, message, 1);
	}
	else if (0==strcmp("io.markusde.epaper", interface_name) && 0==strcmp("flush", member_name)) {
		printf("[server] Enqueuing flush...\n");	
		EnqCommand(d_doflush, NULL);
		reply_result(connection, message, 1);
	}
	else if (0==strcmp("io.markusde.epaper", interface_name) && 0==strcmp("apply", member_name)) {
		printf("[server] Enqueuing apply...\n");	
		respond_to_apply(connection, message);
	}
	else {
		printf("\t[server] Invalid Command\n");
		reply_result(connection, message, 0);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

		
	// Return a successful result	
	return DBUS_HANDLER_RESULT_HANDLED;
}


// ============================================================================
// ==	MAIN
//

int main(void)
{
	// Exception handling:ctrl + c
    signal(SIGINT, Handler);

	// Start up the queue and mutexes
  	command_buffer = StsQueue.create();
	queue_nonempty = 0;
	pthread_mutex_init(&mx_queue_nonempty, NULL);
	pthread_cond_init(&cv_queue_nonempty, NULL);
	
	// Set up DBus connection	
	DBusConnection *connection = NULL;
	DBusError *error = NULL;
	DBusObjectPathVTable vtable;
	dbus_error_init(&error);

	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	check_and_abort(&error);

	dbus_bus_request_name(connection, "io.markusde.epaper", 0, &error);
	check_and_abort(&error);

	vtable.message_function = draw_messages;
	vtable.unregister_function = NULL;
	
	dbus_connection_try_register_object_path(connection,
				"/io/markusde/epaper",
				&vtable,
				NULL,
				&error);
	check_and_abort(&error);

	// Start up the thread
	if(pthread_create(&thread_executor, NULL, exec, NULL)) {
		printf("[server] Error creating thread\n");
		return 1;
	}
	
	// Main loop
	while(SERV_RUNNING) {
			dbus_connection_read_write_dispatch(connection, 1000);
	}
	exit_clean();
    return 0;
}


static void respond_to_introspect(DBusConnection *connection, DBusMessage *request) {
	DBusMessage *reply;
	const char *introspection_data =
		" <!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
		"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
		" <!-- dbus-sharp 0.8.1 -->"
		" <node>"
		"   <interface name=\"org.freedesktop.DBus.Introspectable\">"
		"     <method name=\"Introspect\">"
		"       <arg name=\"data\" direction=\"out\" type=\"s\" />"
		"     </method>"
		"   </interface>"
		"	<interface name=\"io.markusde.epaper\">"
		"	  <method name=\"setup\">"
		"       <arg name=\"result\" direction=\"out\" type=\"b\" />"
		"	  </method>"
		"	  <method name=\"close\">"
		"       <arg name=\"result\" direction=\"out\" type=\"b\" />"
		"	  </method>"
		"	  <method name=\"clear\">"
		"       <arg name=\"result\" direction=\"out\" type=\"b\" />"
		"	  </method>"
		"	  <method name=\"push\">"
		"       <arg name=\"result\" direction=\"out\" type=\"b\" />"
		"	  </method>"
		"	  <method name=\"flush\">"
		"       <arg name=\"result\" direction=\"out\" type=\"b\" />"
		"	  </method>"
		"	  <method name=\"apply\">"
		"		<arg name=\"data\" direction=\"in\" type=\"a(qqqqqyyyys)\"/>"
		"       <arg name=\"result\" direction=\"out\" type=\"b\" />"
		"	  </method>"
		"	  <method name=\"serv_close\">"
		"       <arg name=\"result\" direction=\"out\" type=\"b\" />"
		"	  </method>"
		"	</interface>"
		" </node>";
	
	reply = dbus_message_new_method_return(request);
	dbus_message_append_args(reply, DBUS_TYPE_STRING, &introspection_data, DBUS_TYPE_INVALID);
	dbus_connection_send(connection, reply, NULL);
	dbus_message_unref(reply);
}


static void respond_to_apply(DBusConnection *connection, DBusMessage *request) { 
	DBusError error;
	DBusMessageIter msgIter;

	dbus_error_init(&error);
	dbus_message_iter_init(request, &msgIter);
	
	if (dbus_error_is_set(&error)) {
			printf("	Apply has setup error\n");
		return NULL;
	}


	if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&msgIter)) {
		DBusMessageIter structIter;
		dbus_message_iter_recurse(&msgIter, &structIter);

		for (int i = 0; i < dbus_message_iter_get_element_count(&msgIter); i++) {
			DBusMessageIter innerStructIter;
			dbus_message_iter_recurse(&structIter, &innerStructIter);

			// Get the struct values
			apply_args_t *args = malloc(sizeof(apply_args_t));
			dbus_message_iter_get_basic (&innerStructIter, &args->command);		
			dbus_message_iter_next(&innerStructIter);
			dbus_message_iter_get_basic (&innerStructIter, &args->x0);		
			dbus_message_iter_next(&innerStructIter);
			dbus_message_iter_get_basic (&innerStructIter, &args->y0);		
			dbus_message_iter_next(&innerStructIter);
			dbus_message_iter_get_basic (&innerStructIter, &args->x1);		
			dbus_message_iter_next(&innerStructIter);
			dbus_message_iter_get_basic (&innerStructIter, &args->y1);		
			dbus_message_iter_next(&innerStructIter);
			dbus_message_iter_get_basic (&innerStructIter, &args->col_f);		
			dbus_message_iter_next(&innerStructIter);
			dbus_message_iter_get_basic (&innerStructIter, &args->col_b);		
			dbus_message_iter_next(&innerStructIter);
			dbus_message_iter_get_basic (&innerStructIter, &args->dot_w);		
			dbus_message_iter_next(&innerStructIter);
			dbus_message_iter_get_basic (&innerStructIter, &args->aux);		
			dbus_message_iter_next(&innerStructIter);
			dbus_message_iter_get_basic (&innerStructIter, &args->dat);		
			dbus_message_iter_next(&innerStructIter);
		
			printf("[server] Read apply command:	%d %d %d %d %d - %d %d %d %d - %s\n", args->command, args->x0, args->y0, args->x1, args->y1, args->col_f, args->col_b, args->dot_w, args->aux, args->dat);
			EnqCommand(d_doapply, args);
			dbus_message_iter_next(&structIter);
		}
	}
	
	reply_result(connection, request, 1);
}

