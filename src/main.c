#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mqtt.h"

void publish_callback(void **unused, struct mqtt_response_publish *published);

void *client_refresher(void *client);

void exit_example(int status, int sockfd, pthread_t *client_daemon);

int main(int argc, int *argv[])
{
    const char *addr;
    const char *port;
    const char *topic;

    addr = "test.moquitto.org";
    port = "1883";
    topic = "datetime";

    int sockfd = open_nb_socket(addr, port);

    if (sockfd == -1)
    {
        perror("Failed to open socket: ");
        exit_example(EXIT_FAILURE, sockfd, NULL);
    }

    struct mqtt_client client;
    uint8_t sendbuf[2048];
    uint8_t recvbuf[1024];
    mqtt_init(&client, sockfd, sendbuf, sizeof(sendbuf), recvbuf, sizeof(recvbuf), publish_callback);
    mqtt_connect(&client, "publishing_client", NULL, NULL, 0, NULL, NULL, 0, 400);

    /* check that we don't have any errors */
    if (client.error != MQTT_OK)
    {
        fprintf(stderr, "error: %s\n", mqtt_error_str(client.error));
        exit_example(EXIT_FAILURE, sockfd, NULL);
    }
    /* start a thread to refresh the client (handle egress and ingree client traffic) */
    pthread_t client_daemon;
    if (pthread_create(&client_daemon, NULL, client_refresher, &client))
    {
        fprintf(stderr, "Failed to start client daemon.\n");
        exit_example(EXIT_FAILURE, sockfd, NULL);
    }
    /* start publishing the time */
    printf("%s is ready to begin publishing the time.\n", argv[0]);
    printf("Press ENTER to publish the current time.\n");
    printf("Press CTRL-D (or any other key) to exit.\n\n");
    while (fgetc(stdin) == '\n')
    {
        /* get the current time */
        time_t timer;
        time(&timer);
        struct tm *tm_info = localtime(&timer);
        char timebuf[26];
        strftime(timebuf, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        /* print a message */
        char application_message[256];
        snprintf(application_message, sizeof(application_message), "The time is %s", timebuf);
        printf("%s published : \"%s\"", argv[0], application_message);
        /* publish the time */
        mqtt_publish(&client, topic, application_message, strlen(application_message) + 1, MQTT_PUBLISH_QOS_0);
        /* check for errors */
        if (client.error != MQTT_OK)
        {
            fprintf(stderr, "error: %s\n", mqtt_error_str(client.error));
            exit_example(EXIT_FAILURE, sockfd, &client_daemon);
        }
    }
    /* disconnect */
    printf("\n%s disconnecting from %s\n", argv[0], addr);
    sleep(1);
    /* exit */
    exit_example(EXIT_SUCCESS, sockfd, &client_daemon);
}

void exit_example(int status, int sockfd, pthread_t *client_daemon)
{
    if (sockfd != -1)
        close(sockfd);
    if (client_daemon != NULL)
        pthread_cancel(*client_daemon);
    exit(status);
}
void publish_callback(void **unused, struct mqtt_response_publish *published)
{
    /* not used in this example */
}
void *client_refresher(void *client)
{
    while (1)
    {
        mqtt_sync((struct mqtt_client *)client);
        usleep(100000U);
    }
    return NULL;
}