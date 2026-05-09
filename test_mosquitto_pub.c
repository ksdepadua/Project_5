#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>

// To compile: gcc -o test_mosquitto_pub test_mosquitto_pub.c -lmosquitto

int main() {
	int rc;
	struct mosquitto * mosq; // info for the client

	mosquitto_lib_init();

	mosq = mosquitto_new("publisher-test", true, NULL);

	rc = mosquitto_connect(mosq, "localhost", 1883, 60);
	if(rc != 0) {
		printf("Client couldn't connect to broker. Error Code: %d\n", rc);
		mosquitto_destroy(mosq);
		return -1;
	}

	printf("We are now connected to the broker!\n");

	mosquitto_publish(mosq, NULL, "test/t1", 6, "Hello", 0, false);

	mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);

	mosquitto_lib_cleanup();

	return 0;
}
