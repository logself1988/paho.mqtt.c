#define MY_ID "@(#) stmqcom/xrctest5.c, stmqcom, cs 1.85 12/11/29 07:36:24"
/*--------------------------------------------------------------------*/
/* [Platforms]UNIX NT[/Platforms]                                     */
/* [Title]MQ Telemetry MQTT Asynchronous C client SSL tests           */
/* [/Title]                                                           */
/* [Testclasses]stcom1 stmqcom1[/Category]                            */
/* [Category]MQ Telemetry[/Category]                                  */
/*                                                                    */
/* Copyright IBM 2012                                                 */
/* All rights reserved.                                               */
/*--------------------------------------------------------------------*/

/**
 * @file
 * SSL tests for the MQ Telemetry Asynchronous MQTT C client
 */

#if !defined(_WINDOWS)
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#define MAXHOSTNAMELEN 256
#define EAGAIN WSAEWOULDBLOCK
#define EINTR WSAEINTR
#define EINPROGRESS WSAEINPROGRESS
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ENOTCONN WSAENOTCONN
#define ECONNRESET WSAECONNRESET
#define snprintf _snprintf
#endif

#include "MQTTAsync.h"
#include <string.h>
#include <stdlib.h>

#if defined(IOS)
char skeytmp[1024];
char ckeytmp[1024];
char persistenceStore[1024];
#else
char* persistenceStore = NULL;
#endif

#if 0
#include <logaX.h>   /* For general log messages                      */
#define MyLog logaLine
#else
#define LOGA_DEBUG 0
#define LOGA_INFO 1
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void usage()
{
	printf("Options:\n");
    printf("\t--test_no <test_no> - Run test number <test_no>\n");
    printf("\t--server <mqtt URL> - Connect to <mqtt URL> for tests\n");
    printf("\t--haserver <mqtt URL> - Use <mqtt URL> as the secondary server for HA tests\n");
    printf("\t--client_key <key_file> - Use <key_file> as the client certificate for SSL authentication\n");
    printf("\t--client_key_pass <password> - Use <password> to access the private key in the client certificate\n");
    printf("\t--server_key <key_file> - Use <key_file> as the trusted certificate for server\n");
    printf("\t--verbose - Enable verbose output \n");
    printf("\tserver connection URLs should be in the form; (tcp|ssl)://hostname:port\n");
    printf("\t--help - This help output\n");
	exit(-1);
}

struct Options
{
	char* connection; /**< connection to system under test. */
	char** haconnections;
	int hacount;
	char* client_key_file;
	char* client_key_pass;
	char* server_key_file;
	int verbose;
	int test_no;
	int size;
} options =
{
	"ssl://localhost:8883",
	NULL,
	0,
	NULL,
	NULL,
	NULL,
	0,
	0,
	5000000
};

typedef struct
{
	MQTTAsync client;
	char clientid[24];
	char topic[100];
	int maxmsgs;
	int rcvdmsgs[3];
	int sentmsgs[3];
	int testFinished;
	int subscribed;
} AsyncTestClient;

#define AsyncTestClient_initializer {NULL, "\0", "\0", 0, {0, 0, 0}, {0, 0, 0}, 0, 0}

void getopts(int argc, char** argv)
{
	int count = 1;

	while (count < argc)
	{
		if (strcmp(argv[count], "--help") == 0)
		{
			usage();
		}
		else if (strcmp(argv[count], "--test_no") == 0)
		{
			if (++count < argc)
				options.test_no = atoi(argv[count]);
			else
				usage();
		}
		else if (strcmp(argv[count], "--connection") == 0)
		{
			if (++count < argc)
				options.connection = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--haconnections") == 0)
		{
			if (++count < argc)
			{
				char* tok = strtok(argv[count], " ");
				options.hacount = 0;
				options.haconnections = malloc(sizeof(char*) * 5);
				while (tok)
				{
					options.haconnections[options.hacount] = malloc(strlen(tok) + 1);
					strcpy(options.haconnections[options.hacount], tok);
					options.hacount++;
					tok = strtok(NULL, " ");
				}
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "--client_key") == 0)
		{
			if (++count < argc)
            {
#if defined(IOS)
                strcat(ckeytmp, getenv("HOME"));
                strcat(ckeytmp, argv[count]);
                options.client_key_file = ckeytmp;
#else
                options.client_key_file = argv[count];
#endif
            }
			else
				usage();
		}
		else if (strcmp(argv[count], "--client_key_pass") == 0)
		{
			if (++count < argc)
				options.client_key_pass = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--server_key") == 0)
		{
			if (++count < argc)
            {
#if defined(IOS)
                strcat(skeytmp, getenv("HOME"));
                strcat(skeytmp, argv[count]);
                options.server_key_file = skeytmp;
#else
				options.server_key_file = argv[count];
#endif
            }
            else
				usage();
		}
		else if (strcmp(argv[count], "--verbose") == 0)
		{
			options.verbose = 1;
			//TODO
			printf("\nSetting verbose on\n");
		}
		count++;
	}
#if defined(IOS)
    strcpy(persistenceStore, getenv("HOME"));
    strcat(persistenceStore, "/Library/Caches");
#endif
}

void MyLog(int LOGA_level, char* format, ...)
{
	static char msg_buf[256];
	va_list args;
	struct timeb ts;

	struct tm *timeinfo;

	if (LOGA_level == LOGA_DEBUG && options.verbose == 0)
		return;

	ftime(&ts);
	timeinfo = localtime(&ts.time);
	strftime(msg_buf, 80, "%Y%m%d %H%M%S", timeinfo);

	sprintf(&msg_buf[strlen(msg_buf)], ".%.3hu ", ts.millitm);

	va_start(args, format);
	vsnprintf(&msg_buf[strlen(msg_buf)], sizeof(msg_buf) - strlen(msg_buf),
			format, args);
	va_end(args);

	printf("%s\n", msg_buf);
	fflush(stdout);
}
#endif

#if defined(WIN32) || defined(_WINDOWS)
#define mqsleep(A) Sleep(1000*A)
#define START_TIME_TYPE DWORD
static DWORD start_time = 0;
START_TIME_TYPE start_clock(void)
{
	return GetTickCount();
}
#elif defined(AIX)
#define mqsleep sleep
#define START_TIME_TYPE struct timespec
START_TIME_TYPE start_clock(void)
{
	static struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);
	return start;
}
#else
#define mqsleep sleep
#define START_TIME_TYPE struct timeval
/* TODO - unused - remove? static struct timeval start_time; */
START_TIME_TYPE start_clock(void)
{
	struct timeval start_time;
	gettimeofday(&start_time, NULL);
	return start_time;
}
#endif

#if defined(WIN32)
long elapsed(START_TIME_TYPE start_time)
{
	return GetTickCount() - start_time;
}
#elif defined(AIX)
#define assert(a)
long elapsed(struct timespec start)
{
	struct timespec now, res;

	clock_gettime(CLOCK_REALTIME, &now);
	ntimersub(now, start, res);
	return (res.tv_sec)*1000L + (res.tv_nsec)/1000000L;
}
#else
long elapsed(START_TIME_TYPE start_time)
{
	struct timeval now, res;

	gettimeofday(&now, NULL);
	timersub(&now, &start_time, &res);
	return (res.tv_sec) * 1000 + (res.tv_usec) / 1000;
}
#endif
START_TIME_TYPE global_start_time;

#define assert(a, b, c, d) myassert(__FILE__, __LINE__, a, b, c, d)
#define assert1(a, b, c, d, e) myassert(__FILE__, __LINE__, a, b, c, d, e)

#define MAXMSGS 30;

int tests = 0;
int failures = 0;

int myassert(char* filename, int lineno, char* description, int value,
		char* format, ...)
{
	++tests;
	if (!value)
	{
		va_list args;

		++failures;
		printf("Assertion failed, file %s, line %d, description: %s", filename, lineno, description);

		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}
	else
		MyLog(LOGA_DEBUG,
				"Assertion succeeded, file %s, line %d, description: %s",
				filename, lineno, description);
	return value;
}

/*********************************************************************

 Async Callbacks - generic callbacks for send/receive tests

 *********************************************************************/

void asyncTestOnDisconnect(void* context, MQTTAsync_successData* response)
{
    AsyncTestClient* tc = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In asyncTestOnDisconnect callback, %s", tc->clientid);
	tc->testFinished = 1;
}

void asyncTestOnDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
    AsyncTestClient* tc = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In asyncTestOnDisconnectFailure callback, %s", tc->clientid);
    failures++;
	tc->testFinished = 1;
}

void asyncTestOnSend(void* context, MQTTAsync_successData* response)
{
	AsyncTestClient* tc = (AsyncTestClient*) context;
	//int qos = response->alt.pub.message.qos;
	MyLog(LOGA_DEBUG, "In asyncTestOnSend callback, %s", tc->clientid);
}

void asyncTestOnSendFailure(void* context, MQTTAsync_failureData* response)
{
    AsyncTestClient* tc = (AsyncTestClient*) context;
    MyLog(LOGA_DEBUG, "In asyncTestOnSendFailure callback, %s", tc->clientid);
    tc->testFinished = 1;
    failures++;
}

void asyncTestOnSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* tc = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In asyncTestOnSubscribeFailure callback, %s",
			tc->clientid);
	assert("There should be no failures in this test. ", 0, "asyncTestOnSubscribeFailure callback was called\n", 0);
    tc->testFinished = 1;
    failures++;
}

void asyncTestOnUnsubscribe(void* context, MQTTAsync_successData* response)
{
	AsyncTestClient* tc = (AsyncTestClient*) context;
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	int rc;

	MyLog(LOGA_DEBUG, "In asyncTestOnUnsubscribe callback, %s", tc->clientid);
	opts.onSuccess = asyncTestOnDisconnect;
    opts.onFailure = asyncTestOnDisconnectFailure;
	opts.context = tc;

	MQTTAsync_disconnect(tc->client, &opts);
}

void asyncTestOnUnsubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* tc = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In asyncTestOnUnsubscribeFailure callback, %s", tc->clientid);
	assert("There should be no failures in this test. ", 0, "asyncTestOnUnsubscribeFailure callback was called\n", 0);
    tc->testFinished = 1;
    failures++;
}

void asyncTestOnSubscribe(void* context, MQTTAsync_successData* response)
{
	AsyncTestClient* tc = (AsyncTestClient*) context;
	int rc, i;
	MyLog(LOGA_DEBUG, "In asyncTestOnSubscribe callback, %s", tc->clientid);

	tc->subscribed = 1;
	
	for (i = 0; i < 3; i++)
	{
		MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
		MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

		pubmsg.payload
				= "a much longer message that we can shorten to the extent that we need to payload up to 11";
		pubmsg.payloadlen = 11;
		pubmsg.qos = i;
		pubmsg.retained = 0;

		opts.onSuccess = asyncTestOnSend;
        opts.onFailure = asyncTestOnSendFailure;
		opts.context = tc;

		rc = MQTTAsync_send(tc->client, tc->topic, pubmsg.payloadlen,
				pubmsg.payload, pubmsg.qos, pubmsg.retained, &opts);
		assert("Good rc from publish", rc == MQTTASYNC_SUCCESS, "rc was %d", rc);
		tc->sentmsgs[i]++;
		MyLog(LOGA_DEBUG, "Maxmsgs %d", tc->maxmsgs);
	}
}

int asyncTestMessageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* m)
{
	int rc;
    
	AsyncTestClient* tc = (AsyncTestClient*) context;
    tc->rcvdmsgs[m->qos]++;

	//printf("Received messages: %d\n", tc->rcvdmsgs[m->qos]);

	MyLog(
			LOGA_DEBUG,
			"In asyncTestMessageArrived callback, %s total to exit %d, total received %d,%d,%d",
			tc->clientid, (tc->maxmsgs * 3), tc->rcvdmsgs[0], tc->rcvdmsgs[1],
			tc->rcvdmsgs[2]);

	if (tc->sentmsgs[m->qos] < tc->maxmsgs)
	{
		MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
		MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

		opts.onSuccess = asyncTestOnSend;
        opts.onFailure = asyncTestOnSendFailure;
		opts.context = tc;

		pubmsg.payload
				= "a much longer message that we can shorten to the extent that we need to payload up to 11";
		pubmsg.payloadlen = 11;
		pubmsg.qos = m->qos;
		pubmsg.retained = 0;

		rc = MQTTAsync_send(tc->client, tc->topic, pubmsg.payloadlen,
				pubmsg.payload, pubmsg.qos, pubmsg.retained, &opts);
		assert("Good rc from publish", rc == MQTTASYNC_SUCCESS, "rc was %d messages sent %d,%d,%d", rc);
		MyLog(LOGA_DEBUG, "Messages sent %d,%d,%d", tc->sentmsgs[0],
				tc->sentmsgs[1], tc->sentmsgs[2]);
		tc->sentmsgs[m->qos]++;
	}
	if ((tc->rcvdmsgs[0] + tc->rcvdmsgs[1] + tc->rcvdmsgs[2]) == (tc->maxmsgs
			* 3))
	{
		MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
		MyLog(LOGA_DEBUG, "Ready to unsubscribe");

		opts.onSuccess = asyncTestOnUnsubscribe;
        opts.onFailure = asyncTestOnUnsubscribeFailure;
		opts.context = tc;
		rc = MQTTAsync_unsubscribe(tc->client, tc->topic, &opts);
		assert("Unsubscribe successful", rc == MQTTASYNC_SUCCESS, "rc was %d", rc);
	}
	MyLog(LOGA_DEBUG, "Leaving asyncTestMessageArrived callback");
	MQTTAsync_freeMessage(&m);
	MQTTAsync_free(topicName);
	return 1;
}

void asyncTestOnDeliveryComplete(void* context, MQTTAsync_token token)
{

}

void asyncTestOnConnect(void* context, MQTTAsync_successData* response)
{
	AsyncTestClient* tc = (AsyncTestClient*) context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int subsqos = 2;
	int rc;

	MyLog(LOGA_DEBUG, "In asyncTestOnConnect callback, %s", tc->clientid);

	opts.onSuccess = asyncTestOnSubscribe;
	opts.onFailure = asyncTestOnSubscribeFailure;
	opts.context = tc;

	rc = MQTTAsync_subscribe(tc->client, tc->topic, subsqos, &opts);
	assert("Good rc from subscribe", rc == MQTTASYNC_SUCCESS, "rc was %d", rc);
}

/*********************************************************************

 Test1: SSL connection to non SSL MQTT server

 *********************************************************************/

int test1Finished = 0;

void test1OnFailure(void* context, MQTTAsync_failureData* response)
{
	MyLog(LOGA_DEBUG, "In connect onFailure callback, context %p", context);
	test1Finished = 1;
}

void test1OnConnect(void* context, MQTTAsync_successData* response)
{

	MyLog(LOGA_DEBUG, "In connect onSuccess callback, context %p\n", context);
	assert("Connect should not succeed", 0, "connect success callback was called", 0);
	test1Finished = 1;
	failures++;
}

int test1(struct Options options)
{
	char* testname = "test1";
	int subsqos = 2;
	MQTTAsync c;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int rc = 0;
	char* test_topic = "C client SSL test1";
	int count = 0;

	test1Finished = 0;
	failures = 0;
	MyLog(LOGA_INFO, "Starting SSL test 1 - connection to nonSSL MQTT server");

	if (!(assert ("Good rc from create", (rc = MQTTAsync_create(&c, options.connection, "test1", MQTTCLIENT_PERSISTENCE_DEFAULT, persistenceStore)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;

	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = "testuser";
	opts.password = "testpassword";
	opts.onSuccess = test1OnConnect;
	opts.onFailure = test1OnFailure;
	opts.context = c;
	if (options.haconnections != NULL)
	{
		opts.serverURIs = options.haconnections;
		opts.serverURIcount = options.hacount;
	}

	opts.ssl = &sslopts;
	opts.ssl->enableServerCertAuth = 0;

	MyLog(LOGA_DEBUG, "Connecting");
	if (!(assert("Good rc from connect", (rc = MQTTAsync_connect(c, &opts)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;

	/* wait for success or failure callback */
	while (!test1Finished && ++count < 10000)
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif

exit:
	MQTTAsync_destroy(&c);
	MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

/*********************************************************************

 Test2a: Mutual SSL Authentication - Certificates in place on client and server

 *********************************************************************/

void test2aOnConnectFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In test2aOnConnectFailure callback, %s",
			client->clientid);

	assert("There should be no failures in this test. ", 0, "test2aOnConnectFailure callback was called\n", 0);
	client->testFinished = 1;
    failures++;
}

void test2aOnPublishFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In test2aOnPublishFailure callback, %s",
			client->clientid);

	assert("There should be no failures in this test. ", 0, "test2aOnPublishFailure callback was called\n", 0);
    client->testFinished = 1;
    failures++;
}

int test2a(struct Options options)
{
	char* testname = "test2a";

	AsyncTestClient tc = AsyncTestClient_initializer;
	MQTTAsync c;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int i, rc = 0;

	failures = 0;

	MyLog(LOGA_INFO, "Starting test 2a - Mutual SSL authentication");
	if (!(assert("Good rc from create", (rc = MQTTAsync_create(&c, options.connection, "test2a", MQTTCLIENT_PERSISTENCE_DEFAULT, persistenceStore)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;

	tc.client = c;
	sprintf(tc.clientid, "%s", testname);
	sprintf(tc.topic, "C client SSL test2a");
	tc.maxmsgs = MAXMSGS;
	//tc.rcvdmsgs = 0;
	tc.subscribed = 0;
	tc.testFinished = 0;

	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = "testuser";
	opts.password = "testpassword";
	opts.onSuccess = asyncTestOnConnect;
	opts.onFailure = test2aOnConnectFailure;
	opts.context = &tc;
	if (options.haconnections != NULL)
	{
		opts.serverURIs = options.haconnections;
		opts.serverURIcount = options.hacount;
	}

	opts.ssl = &sslopts;
	if (options.server_key_file != NULL)
		opts.ssl->trustStore = options.server_key_file; /*file of certificates trusted by client*/
	opts.ssl->keyStore = options.client_key_file; /*file of certificate for client to present to server*/
	if (options.client_key_pass != NULL)
		opts.ssl->privateKeyPassword = options.client_key_pass;
	//opts.ssl->enabledCipherSuites = "DEFAULT";
	//opts.ssl->enabledServerCertAuth = 1;

	if (!(assert("Good rc from setCallbacks", (rc = MQTTAsync_setCallbacks(c, &tc, NULL, asyncTestMessageArrived, asyncTestOnDeliveryComplete)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;

	MyLog(LOGA_DEBUG, "Connecting");

	if (!(assert("Good rc from connect", (rc = MQTTAsync_connect(c, &opts)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;

	while (!tc.testFinished)
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif

	MyLog(LOGA_DEBUG, "Stopping");

exit:
    MQTTAsync_destroy(&c);
	MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

/*********************************************************************

 Test2b: Mutual SSL Authentication - Server does not have Client cert

 *********************************************************************/

int test2bFinished;

void test2bOnConnectFailure(void* context, MQTTAsync_failureData* response)
{
	MyLog(LOGA_DEBUG, "In test2bOnConnectFailure callback, context %p", context);

	assert("This test should call test2bOnConnectFailure. ", 1, "test2bOnConnectFailure callback was called\n", 1);
	test2bFinished = 1;
}

void test2bOnConnect(void* context, MQTTAsync_successData* response)
{
	MyLog(LOGA_DEBUG, "In test2bOnConnectFailure callback, context %p", context);

	assert("This connect should not succeed. ", 0, "test2bOnConnect callback was called\n", 0);
    failures++;
	test2bFinished = 1;
}

int test2b(struct Options options)
{
	char* testname = "test2b";
	int subsqos = 2;
	MQTTAsync c;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int rc = 0;
	int count = 0;

	test2bFinished = 0;
	failures = 0;
	MyLog(
			LOGA_INFO,
			"Starting test 2b - connection to SSL MQTT server with clientauth=req but server does not have client cert");

	if (!(assert("good rc from create", (rc = MQTTAsync_create(&c, options.connection, "test2b", MQTTCLIENT_PERSISTENCE_DEFAULT, persistenceStore)) == MQTTASYNC_SUCCESS, "rc was %d\n", rc)))
		goto exit;

	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = "testuser";
	opts.password = "testpassword";
	opts.onSuccess = test2bOnConnect;
	opts.onFailure = test2bOnConnectFailure;
	opts.context = c;
	if (options.haconnections != NULL)
	{
		opts.serverURIs = options.haconnections;
		opts.serverURIcount = options.hacount;
	}

	opts.ssl = &sslopts;
	if (options.server_key_file != NULL)
		opts.ssl->trustStore = options.server_key_file; /*file of certificates trusted by client*/
	opts.ssl->keyStore = options.client_key_file; /*file of certificate for client to present to server*/
	if (options.client_key_pass != NULL)
		opts.ssl->privateKeyPassword = options.client_key_pass;
	//opts.ssl->enabledCipherSuites = "DEFAULT";
	//opts.ssl->enabledServerCertAuth = 0;

	MyLog(LOGA_DEBUG, "Connecting");

	if (!(assert("Good rc from connect", (rc = MQTTAsync_connect(c, &opts)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;
        
	while (!test2bFinished && ++count < 10000)
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif

exit:
	MQTTAsync_destroy(&c);
	MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

/*********************************************************************

 Test2c: Mutual SSL Authentication - Client does not have Server cert

 *********************************************************************/

int test2cFinished;

void test2cOnConnectFailure(void* context, MQTTAsync_failureData* response)
{
	MyLog(LOGA_DEBUG, "In test2cOnConnectFailure callback, context %p", context);

	assert("This test should call test2cOnConnectFailure. ", 1, "test2cOnConnectFailure callback was called\n", 0);
	test2cFinished = 1;
}

void test2cOnConnect(void* context, MQTTAsync_successData* response)
{
	MyLog(LOGA_DEBUG, "In test2cOnConnectFailure callback, context %p", context);

	assert("This connect should not succeed. ", 0, "test2cOnConnect callback was called\n", 0);
    failures++;
	test2cFinished = 1;
}

int test2c(struct Options options)
{
	char* testname = "test2c";
	char* test_topic = "C client test2c";
	int subsqos = 2;
	MQTTAsync c;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int rc, count = 0;

	test2cFinished = 0;
    failures = 0;
	MyLog(
			LOGA_INFO,
			"Starting test 2c - connection to SSL MQTT server, server auth enabled but unknown cert");

	if (!(assert("good rc from create", (rc = MQTTAsync_create(&c, options.connection, "test2c", MQTTCLIENT_PERSISTENCE_DEFAULT, persistenceStore)) == MQTTASYNC_SUCCESS, "rc was %d\n", rc)))
		goto exit;

	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = "testuser";
	opts.password = "testpassword";
	opts.onSuccess = test2cOnConnect;
	opts.onFailure = test2cOnConnectFailure;
	opts.context = c;
	if (options.haconnections != NULL)
	{
		opts.serverURIs = options.haconnections;
		opts.serverURIcount = options.hacount;
	}

	opts.ssl = &sslopts;
	//if (options.server_key_file != NULL) opts.ssl->trustStore = options.server_key_file; /*file of certificates trusted by client*/
	opts.ssl->keyStore = options.client_key_file; /*file of certificate for client to present to server*/
	if (options.client_key_pass != NULL)
		opts.ssl->privateKeyPassword = options.client_key_pass;
	//opts.ssl->enabledCipherSuites = "DEFAULT";
	//opts.ssl->enabledServerCertAuth = 0;

	MyLog(LOGA_DEBUG, "Connecting");

	if (!(assert("Good rc from connect", (rc = MQTTAsync_connect(c, &opts)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;

	while (!test2cFinished && ++count < 10000)
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif

exit:
	MQTTAsync_destroy(&c);
	MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

/*********************************************************************

 Test3a: Server Authentication - server certificate in client trust store

 *********************************************************************/

void test3aOnConnectFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In test3aOnConnectFailure callback, context %p", context);

	assert("There should be no failures in this test. ", 0, "test3aOnConnectFailure callback was called\n", 0);
    failures++;
	client->testFinished = 1;
}

int test3a(struct Options options)
{
	char* testname = "test3a";
	int subsqos = 2;
	/* TODO - usused - remove ? MQTTAsync_deliveryToken* dt = NULL; */
	AsyncTestClient tc =
	AsyncTestClient_initializer;
	MQTTAsync c;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int i, rc = 0;

	failures = 0;

	MyLog(LOGA_INFO, "Starting test 3a - Server authentication");
	if (!(assert("good rc from create", (rc = MQTTAsync_create(&c, options.connection, "test3a", MQTTCLIENT_PERSISTENCE_DEFAULT, persistenceStore)) == MQTTASYNC_SUCCESS, "rc was %d\n", rc)))
		goto exit;

	tc.client = c;
	sprintf(tc.clientid, "%s", testname);
	sprintf(tc.topic, "C client SSL test3a");
	tc.maxmsgs = MAXMSGS;
	//tc.rcvdmsgs = 0;
	tc.subscribed = 0;
	tc.testFinished = 0;

	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = "testuser";
	opts.password = "testpassword";
	opts.onSuccess = asyncTestOnConnect;
	opts.onFailure = test3aOnConnectFailure;
	opts.context = &tc;
	if (options.haconnections != NULL)
	{
		opts.serverURIs = options.haconnections;
		opts.serverURIcount = options.hacount;
	}

	opts.ssl = &sslopts;
	if (options.server_key_file != NULL)
		opts.ssl->trustStore = options.server_key_file; /*file of certificates trusted by client*/
	//opts.ssl->keyStore = options.client_key_file;  /*file of certificate for client to present to server*/
	//if (options.client_key_pass != NULL) opts.ssl->privateKeyPassword = options.client_key_pass;
	//opts.ssl->enabledCipherSuites = "DEFAULT";
	//opts.ssl->enabledServerCertAuth = 1;


	if (!(assert("Good rc from setCallbacks", (rc = MQTTAsync_setCallbacks(c, &tc, NULL, asyncTestMessageArrived, asyncTestOnDeliveryComplete)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;

	MyLog(LOGA_DEBUG, "Connecting");

	if (!(assert("Good rc from connect", (rc = MQTTAsync_connect(c, &opts)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;
        
	while (!tc.testFinished)
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif

	MyLog(LOGA_DEBUG, "Stopping");

exit:
	MQTTAsync_destroy(&c);
	MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.", (failures
			== 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

/*********************************************************************

 Test3b: Server Authentication - Client does not have server cert

 *********************************************************************/

int test3bFinished;

void test3bOnConnectFailure(void* context, MQTTAsync_failureData* response)
{
	MyLog(LOGA_DEBUG, "In test3bOnConnectFailure callback, context %p", context);

	assert("This test should call test3bOnConnectFailure. ", 1, "test3bOnConnectFailure callback was called\n", 1);
	test3bFinished = 1;
}

void test3bOnConnect(void* context, MQTTAsync_successData* response)
{
	MyLog(LOGA_DEBUG, "In test3bOnConnectFailure callback, context %p", context);

	assert("This connect should not succeed. ", 0, "test3bOnConnect callback was called\n", 0);
    failures++;
	test3bFinished = 1;
}

int test3b(struct Options options)
{
	char* testname = "test3b";
	int subsqos = 2;
	MQTTAsync c;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int count, rc = 0;

	test3bFinished = 0;
	failures = 0;
	MyLog( LOGA_INFO, "Starting test 3b - connection to SSL MQTT server with clientauth=opt but client does not have server cert");

	if (!(assert("good rc from create", (rc = MQTTAsync_create(&c, options.connection, "test3b", MQTTCLIENT_PERSISTENCE_DEFAULT, persistenceStore)) == MQTTASYNC_SUCCESS, "rc was %d\n", rc)))
		goto exit;
        
	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = "testuser";
	opts.password = "testpassword";
	opts.onSuccess = test3bOnConnect;
	opts.onFailure = test3bOnConnectFailure;
	opts.context = c;
	if (options.haconnections != NULL)
	{
		opts.serverURIs = options.haconnections;
		opts.serverURIcount = options.hacount;
	}

	opts.ssl = &sslopts;
	//if (options.server_key_file != NULL) opts.ssl->trustStore = options.server_key_file; /*file of certificates trusted by client*/
	//opts.ssl->keyStore = options.client_key_file;  /*file of certificate for client to present to server*/
	//if (options.client_key_pass != NULL) opts.ssl->privateKeyPassword = options.client_key_pass;
	//opts.ssl->enabledCipherSuites = "DEFAULT";
	//opts.ssl->enabledServerCertAuth = 0;

	MyLog(LOGA_DEBUG, "Connecting");

	if (!(assert("Good rc from connect", (rc = MQTTAsync_connect(c, &opts)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;
        
	while (!test3bFinished && ++count < 10000)
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif

exit:
	MQTTAsync_destroy(&c);
	MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

/*********************************************************************

 Test4: Accept invalid server certificates

 *********************************************************************/

void test4OnConnectFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In test4OnConnectFailure callback, context %p", context);

	assert("There should be no failures in this test. ", 0, "test4OnConnectFailure callback was called\n", 0);
    failures++;
	client->testFinished = 1;
}

void test4OnPublishFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
    MyLog(LOGA_DEBUG, "In test4OnPublishFailure callback, context %p", context);

	assert("There should be no failures in this test. ", 0, "test4OnPublishFailure callback was called\n", 0);
    failures++;
    client->testFinished = 1;
}

int test4(struct Options options)
{
	char* testname = "test4";
	int subsqos = 2;
	/* TODO - usused - remove ? MQTTAsync_deliveryToken* dt = NULL; */
	AsyncTestClient tc = AsyncTestClient_initializer;
	MQTTAsync c;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int i, rc = 0;

	failures = 0;

	MyLog(LOGA_INFO, "Starting test 4 - accept invalid server certificates");
	if (!(assert("Good rc from create", (rc = MQTTAsync_create(&c, options.connection, "test4", MQTTCLIENT_PERSISTENCE_NONE, persistenceStore)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;
    
	tc.client = c;
	sprintf(tc.clientid, "%s", testname);
	sprintf(tc.topic, "C client SSL test4");
	tc.maxmsgs = MAXMSGS;
	//tc.rcvdmsgs = 0;
	tc.subscribed = 0;
	tc.testFinished = 0;

	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = "testuser";
	opts.password = "testpassword";
	opts.onSuccess = asyncTestOnConnect;
	opts.onFailure = test4OnConnectFailure;
	opts.context = &tc;
	if (options.haconnections != NULL)
	{
		opts.serverURIs = options.haconnections;
		opts.serverURIcount = options.hacount;
	}

	opts.ssl = &sslopts;
	//if (options.server_key_file != NULL) opts.ssl->trustStore = options.server_key_file; /*file of certificates trusted by client*/
	//opts.ssl->keyStore = options.client_key_file;  /*file of certificate for client to present to server*/
	//if (options.client_key_pass != NULL) opts.ssl->privateKeyPassword = options.client_key_pass;
	//opts.ssl->enabledCipherSuites = "DEFAULT";
	opts.ssl->enableServerCertAuth = 0;

	if (!(assert("Good rc from setCallbacks", (rc = MQTTAsync_setCallbacks(c, &tc, NULL, asyncTestMessageArrived, asyncTestOnDeliveryComplete)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;

	MyLog(LOGA_DEBUG, "Connecting");

	if (!(assert("Good rc from connect", (rc = MQTTAsync_connect(c, &opts)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;
        
	while (!tc.testFinished)
    {
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif
    }

	MyLog(LOGA_DEBUG, "Stopping");

exit:
    MQTTAsync_destroy(&c);
	MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

/*********************************************************************

 Test5a: Anonymous ciphers - server auth disabled

 *********************************************************************/

void test5aOnConnectFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In test5aOnConnectFailure callback, context %p", context);

	assert("There should be no failures in this test. ", 0, "test5aOnConnectFailure callback was called\n", 0);
    failures++;
	client->testFinished = 1;
}

void test5aOnPublishFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
    MyLog(LOGA_DEBUG, "In test5aOnPublishFailure callback, context %p", context);

	assert("There should be no failures in this test. ", 0, "test5aOnPublishFailure callback was called\n", 0);
    failures++;
    client->testFinished = 1;
}

int test5a(struct Options options)
{
	char* testname = "test5a";

	AsyncTestClient tc = AsyncTestClient_initializer;
	MQTTAsync c;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int i, rc = 0;

	failures = 0;

	MyLog(LOGA_INFO, "Starting SSL test 5a - Anonymous ciphers - server authentication disabled");
	if (!(assert("good rc from create", (rc = MQTTAsync_create(&c, options.connection, "test5a", MQTTCLIENT_PERSISTENCE_DEFAULT, persistenceStore)) == MQTTASYNC_SUCCESS, "rc was %d\n", rc)))
		goto exit;
        
	tc.client = c;
	sprintf(tc.clientid, "%s", testname);
	sprintf(tc.topic, "C client SSL test5a");
	tc.maxmsgs = MAXMSGS;
	//tc.rcvdmsgs = 0;
	tc.subscribed = 0;
	tc.testFinished = 0;

	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = "testuser";
	opts.password = "testpassword";
	opts.onSuccess = asyncTestOnConnect;
	opts.onFailure = test5aOnConnectFailure;
	opts.context = &tc;
	if (options.haconnections != NULL)
	{
		opts.serverURIs = options.haconnections;
		opts.serverURIcount = options.hacount;
	}

	opts.ssl = &sslopts;
	//opts.ssl->trustStore = /*file of certificates trusted by client*/
	//opts.ssl->keyStore = options.client_key_file;  /*file of certificate for client to present to server*/
	//if (options.client_key_pass != NULL) opts.ssl->privateKeyPassword = options.client_key_pass;
	opts.ssl->enabledCipherSuites = "aNULL";
	opts.ssl->enableServerCertAuth = 0;

	if (!(assert("Good rc from setCallbacks", (rc = MQTTAsync_setCallbacks(c, &tc, NULL, asyncTestMessageArrived, asyncTestOnDeliveryComplete)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;

	MyLog(LOGA_DEBUG, "Connecting");

	if (!(assert("Good rc from connect", (rc = MQTTAsync_connect(c, &opts)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;
        
	while (!tc.testFinished)
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif

	MyLog(LOGA_DEBUG, "Stopping");

exit:
    MQTTAsync_destroy(&c);
	MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

/*********************************************************************

 Test5b: Anonymous ciphers - server auth enabled

 /********************************************************************/

void test5bOnConnectFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In test5bOnConnectFailure callback, context %p", context);

	assert("There should be no failures in this test. ", 0, "test5bOnConnectFailure callback was called\n", 0);
    failures++;
	client->testFinished = 1;
}

void test5bOnPublishFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
    MyLog(LOGA_DEBUG, "In test5bOnPublishFailure callback, context %p", context);

	assert("There should be no failures in this test. ", 0, "test5bOnPublishFailure callback was called\n", 0);
    failures++;
    client->testFinished = 1;
}

int test5b(struct Options options)
{
	char* testname = "test5b";

	AsyncTestClient tc =
	AsyncTestClient_initializer;
	MQTTAsync c;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int i, rc = 0;

	failures = 0;

	MyLog(LOGA_INFO,
			"Starting SSL test 5b - Anonymous ciphers - server authentication enabled");
	if (!(assert("good rc from create", (rc = MQTTAsync_create(&c, options.connection, "test5b", MQTTCLIENT_PERSISTENCE_DEFAULT, persistenceStore)) == MQTTASYNC_SUCCESS, "rc was %d\n", rc)))
		goto exit;
        
	tc.client = c;
	sprintf(tc.clientid, "%s", testname);
	sprintf(tc.topic, "C client SSL test5b");
	tc.maxmsgs = MAXMSGS;
	//tc.rcvdmsgs = 0;
	tc.subscribed = 0;
	tc.testFinished = 0;

	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = "testuser";
	opts.password = "testpassword";
	opts.onSuccess = asyncTestOnConnect;
	opts.onFailure = test5bOnConnectFailure;
	opts.context = &tc;
	if (options.haconnections != NULL)
	{
		opts.serverURIs = options.haconnections;
		opts.serverURIcount = options.hacount;
	}

	opts.ssl = &sslopts;
	//opts.ssl->trustStore = /*file of certificates trusted by client*/
	//opts.ssl->keyStore = options.client_key_file;  /*file of certificate for client to present to server*/
	//if (options.client_key_pass != NULL) opts.ssl->privateKeyPassword = options.client_key_pass;
	opts.ssl->enabledCipherSuites = "aNULL";
	opts.ssl->enableServerCertAuth = 1;

	rc = MQTTAsync_setCallbacks(c, &tc, NULL, asyncTestMessageArrived, asyncTestOnDeliveryComplete);
	assert("Good rc from setCallbacks", rc == MQTTASYNC_SUCCESS, "rc was %d", rc);

	MyLog(LOGA_DEBUG, "Connecting");
	rc = MQTTAsync_connect(c, &opts);
	assert("Good rc from connect", rc == MQTTASYNC_SUCCESS, "rc was %d", rc);
	if (rc != MQTTASYNC_SUCCESS)
    {
        failures++;
		goto exit;
    }
        
	while (!tc.testFinished)
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif

	MyLog(LOGA_DEBUG, "Stopping");

exit:
    MQTTAsync_destroy(&c);
	MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

/*********************************************************************

 Test5c: Anonymous ciphers - client not using anonymous ciphers

 *********************************************************************/

int test5cFinished;

void test5cOnConnectFailure(void* context, MQTTAsync_failureData* response)
{
	MyLog(LOGA_DEBUG, "In test5cOnConnectFailure callback, context %p", context);

	assert("This test should call test5cOnConnectFailure. ", 1, "test5cOnConnectFailure callback was called\n", 1);
	test5cFinished = 1;
}

void test5cOnConnect(void* context, MQTTAsync_successData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
    MyLog(LOGA_DEBUG, "In test5cOnConnectFailure callback, context %p", context);

	assert("This connect should not succeed. ", 0, "test5cOnConnect callback was called\n", 0);
    failures++;
	test5cFinished = 1;
}

int test5c(struct Options options)
{
	char* testname = "test5c";
	int subsqos = 2;
	MQTTAsync c;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int rc = 0;
	int count = 0;

	test5cFinished = 0;
	failures = 0;
	MyLog(LOGA_INFO, "Starting SSL test 5c - Anonymous ciphers - client not using anonymous cipher");
	if (!(assert("good rc from create", (rc = MQTTAsync_create(&c, options.connection, "test5c", MQTTCLIENT_PERSISTENCE_DEFAULT, persistenceStore)) == MQTTASYNC_SUCCESS, "rc was %d\n", rc)))
		goto exit;

	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = "testuser";
	opts.password = "testpassword";
	opts.onSuccess = test5cOnConnect;
	opts.onFailure = test5cOnConnectFailure;
	opts.context = c;
	if (options.haconnections != NULL)
	{
		opts.serverURIs = options.haconnections;
		opts.serverURIcount = options.hacount;
	}

	opts.ssl = &sslopts;
	//opts.ssl->trustStore = /*file of certificates trusted by client*/
	//opts.ssl->keyStore = options.client_key_file;  /*file of certificate for client to present to server*/
	//if (options.client_key_pass != NULL) opts.ssl->privateKeyPassword = options.client_key_pass;
	//opts.ssl->enabledCipherSuites = "DEFAULT";
	opts.ssl->enableServerCertAuth = 0;

	MyLog(LOGA_DEBUG, "Connecting");

	assert("Good rc from connect", (rc = MQTTAsync_connect(c, &opts)) == MQTTASYNC_SUCCESS, "rc was %d\n", rc);
	if (rc != MQTTASYNC_SUCCESS)
    {
        failures++;
		goto exit;
    }
        
	while (!test5cFinished && ++count < 10000)
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif

exit:
    MQTTAsync_destroy(&c);
	MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

/*********************************************************************

 Test6: More than one client object - simultaneous working.

 *********************************************************************/

void test6OnConnectFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In test6OnConnectFailure callback, context %p", context);

	assert("There should be no failures in this test. ", 0, "test6OnConnectFailure callback was called\n", 0);
    failures++;
	client->testFinished = 1;
}

void test6OnPublishFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
    MyLog(LOGA_DEBUG, "In test6OnPublishFailure callback, context %p", context);

	assert("There should be no failures in this test. ", 0, "test6OnPublishFailure callback was called\n", 0);
    failures++;
    client->testFinished = 1;
}

int test6(struct Options options)
{
#define NUM_CLIENTS 10
	char* testname = "test6";
	const int num_clients = NUM_CLIENTS;
	int subsqos = 2;
	AsyncTestClient tc[NUM_CLIENTS];
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int rc = 0;
	int i;
	int test6finished = 0;
	failures = 0;

	MyLog(LOGA_INFO, "Starting test 6 - multiple connections");
	for (i = 0; i < num_clients; ++i)
	{
		tc[i].maxmsgs = MAXMSGS;
		tc[i].rcvdmsgs[0] = 0;
		tc[i].rcvdmsgs[1] = 0;
		tc[i].rcvdmsgs[2] = 0;
		tc[i].sentmsgs[0] = 0;
		tc[i].sentmsgs[1] = 0;
		tc[i].sentmsgs[2] = 0;
		tc[i].testFinished = 0;
		sprintf(tc[i].clientid, "sslasync_test6_num_%d", i);
		sprintf(tc[i].topic, "sslasync test6 topic num %d", i);

		if (!(assert("good rc from create", (rc = MQTTAsync_create(&(tc[i].client), options.connection, tc[i].clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL)) == MQTTASYNC_SUCCESS, "rc was %d\n", rc)))
			goto exit;

		if (!(assert("Good rc from setCallbacks", (rc = MQTTAsync_setCallbacks(tc[i].client, &tc[i], NULL, asyncTestMessageArrived, NULL)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
			goto exit;
        
		opts.keepAliveInterval = 20;
		opts.cleansession = 1;
		opts.username = "testuser";
		opts.password = "testpassword";
		opts.onSuccess = asyncTestOnConnect;
		opts.onFailure = test6OnConnectFailure;
		opts.context = &tc[i];
		if (options.haconnections != NULL)
		{
			opts.serverURIs = options.haconnections;
			opts.serverURIcount = options.hacount;
		}

		opts.ssl = &sslopts;
		if (options.server_key_file != NULL)
			opts.ssl->trustStore = options.server_key_file; /*file of certificates trusted by client*/
		opts.ssl->keyStore = options.client_key_file; /*file of certificate for client to present to server*/
		if (options.client_key_pass != NULL)
			opts.ssl->privateKeyPassword = options.client_key_pass;
		//opts.ssl->enabledCipherSuites = "DEFAULT";
		//opts.ssl->enabledServerCertAuth = 1;

		MyLog(LOGA_DEBUG, "Connecting");

		assert("Good rc from connect", (rc = MQTTAsync_connect(tc[i].client, &opts)) == MQTTASYNC_SUCCESS, "rc was %d", rc);
	}

	while (test6finished < num_clients)
	{
		MyLog(LOGA_DEBUG, "num_clients %d test_finished %d\n", num_clients,
				test6finished);
#if defined(WIN32)
		Sleep(100);

#else
		usleep(10000L);
#endif
		for (i = 0; i < num_clients; ++i)
		{
			if (tc[i].testFinished)
			{
				test6finished++;
				tc[i].testFinished = 0;
			}
		}
	}

exit:
	MyLog(LOGA_DEBUG, "test6: destroying clients");

	for (i = 0; i < num_clients; ++i)
		MQTTAsync_destroy(&tc[i].client);

	MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.",
		(failures == 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

/*********************************************************************

 Test7: Send and receive big messages

 *********************************************************************/

void* test7_payload = NULL;
int test7_payloadlen = 0;

void test7OnConnectFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In test7OnConnectFailure callback, %s", client->clientid);

	assert("There should be no failures in this test. ", 0, "test7OnConnectFailure callback was called\n", 0);
    failures++;
	client->testFinished = 1;
}

void test7OnPublishFailure(void* context, MQTTAsync_failureData* response)
{
	AsyncTestClient* client = (AsyncTestClient*) context;
	MyLog(LOGA_DEBUG, "In test7OnPublishFailure callback, %s", client->clientid);

	assert("There should be no failures in this test. ", 0, "test7OnPublishFailure callback was called\n", 0);
    failures++;
	client->testFinished = 1;
}

int test7MessageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message)
{
	AsyncTestClient* tc = (AsyncTestClient*) context;
	static int message_count = 0;
	int rc, i;

	MyLog(LOGA_DEBUG, "In messageArrived callback %p", tc);

	assert("Message size correct", message->payloadlen == test7_payloadlen,
			"message size was %d", message->payloadlen);

	for (i = 0; i < options.size; ++i)
	{
		if (((char*) test7_payload)[i] != ((char*) message->payload)[i])
		{
			assert("Message contents correct", ((char*)test7_payload)[i] != ((char*)message->payload)[i],
					"message content was %c", ((char*)message->payload)[i]);
            failures++;
			break;
		}
	}

	if (++message_count == 1)
	{
		MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
		MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

		pubmsg.payload = test7_payload;
		pubmsg.payloadlen = test7_payloadlen;
		pubmsg.qos = 1;
		pubmsg.retained = 0;
		opts.onSuccess = NULL;
		opts.onFailure = test7OnPublishFailure;
		opts.context = tc;

		rc = MQTTAsync_sendMessage(tc->client, tc->topic, &pubmsg, &opts);
	}
	else if (message_count == 2)
	{
		MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
		MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

		pubmsg.payload = test7_payload;
		pubmsg.payloadlen = test7_payloadlen;
		pubmsg.qos = 0;
		pubmsg.retained = 0;
		opts.onSuccess = NULL;
		opts.onFailure = test7OnPublishFailure;
		opts.context = tc;
		rc = MQTTAsync_sendMessage(tc->client, tc->topic, &pubmsg, &opts);
	}
	else
	{
		MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

		opts.onSuccess = asyncTestOnUnsubscribe;
		opts.context = tc;
		rc = MQTTAsync_unsubscribe(tc->client, tc->topic, &opts);
		assert("Unsubscribe successful", rc == MQTTASYNC_SUCCESS, "rc was %d", rc);
        message_count = 0;
	}

	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);

	return 1;
}

void test7OnSubscribe(void* context, MQTTAsync_successData* response)
{
	AsyncTestClient* tc = (AsyncTestClient*) context;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc, i;

	MyLog(LOGA_DEBUG, "In subscribe onSuccess callback %p", tc);

	pubmsg.payload = test7_payload = malloc(options.size);
	pubmsg.payloadlen = test7_payloadlen = options.size;

	srand(33);
	for (i = 0; i < options.size; ++i)
		((char*) pubmsg.payload)[i] = rand() % 256;

	pubmsg.qos = 2;
	pubmsg.retained = 0;

	rc = MQTTAsync_send(tc->client, tc->topic, pubmsg.payloadlen, pubmsg.payload,
			pubmsg.qos, pubmsg.retained, NULL);
}

void test7OnConnect(void* context, MQTTAsync_successData* response)
{
	AsyncTestClient* tc = (AsyncTestClient*) context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;

	MyLog(LOGA_DEBUG, "In connect onSuccess callback, context %p", context);
	opts.onSuccess = test7OnSubscribe;
	opts.context = tc;


	if (!(assert("Good rc from subscribe", (rc = MQTTAsync_subscribe(tc->client, tc->topic, 2, &opts)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		tc->testFinished = 1;
}

int test7(struct Options options)
{
	char* testname = "test7";
	int subsqos = 2;
	AsyncTestClient tc =
	AsyncTestClient_initializer;
	MQTTAsync c;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
	MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
	int rc = 0;
	char* test_topic = "C client test7";
	int test_finished;

    failures = 0;
	MyLog(LOGA_INFO, "Starting test 7 - big messages");
	if (!(assert("good rc from create", (rc = MQTTAsync_create(&c, options.connection, "async_test_7", MQTTCLIENT_PERSISTENCE_NONE, NULL)) == MQTTASYNC_SUCCESS, "rc was %d\n", rc)))
		goto exit;

	if (!(assert("Good rc from setCallbacks", (rc = MQTTAsync_setCallbacks(c, &tc, NULL, test7MessageArrived, NULL)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;

	tc.client = c;
	sprintf(tc.clientid, "%s", testname);
	sprintf(tc.topic, "C client SSL test7");
	tc.maxmsgs = MAXMSGS;
	//tc.rcvdmsgs = 0;
	tc.subscribed = 0;
	tc.testFinished = 0;

	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = "testuser";
	opts.password = "testpassword";
	opts.onSuccess = test7OnConnect;
	opts.onFailure = test7OnConnectFailure;
	opts.context = &tc;
	if (options.haconnections != NULL)
	{
		opts.serverURIs = options.haconnections;
		opts.serverURIcount = options.hacount;
	}

	opts.ssl = &sslopts;
	if (options.server_key_file != NULL)
		opts.ssl->trustStore = options.server_key_file; /*file of certificates trusted by client*/
	opts.ssl->keyStore = options.client_key_file; /*file of certificate for client to present to server*/
	if (options.client_key_pass != NULL)
		opts.ssl->privateKeyPassword = options.client_key_pass;
	//opts.ssl->enabledCipherSuites = "DEFAULT";
	//opts.ssl->enabledServerCertAuth = 1;

	MyLog(LOGA_DEBUG, "Connecting");

	if (!(assert("Good rc from connect", (rc = MQTTAsync_connect(c, &opts)) == MQTTASYNC_SUCCESS, "rc was %d", rc)))
		goto exit;

	while (!tc.testFinished)
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif

exit:
    MQTTAsync_destroy(&c);
    MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", testname, tests, failures);

	return failures;
}

void handleTrace(enum MQTTASYNC_TRACE_LEVELS level, char* message)
{
	//printf("Trace: %d, %s\n", level, message);
}

int main(int argc, char** argv)
{
	int* numtests = &tests;
	int rc = 0;
	int i;
	int (*tests[])() =
	{ NULL, test1, test2a, test2b, test2c, test3a, test3b, test4, test5a,
			test5b, test5c, test6, test7 };
	char** versionInfo = MQTTAsync_getVersionInfo();

	for (i = 0; versionInfo[i] != NULL; i++)
	 {
	 printf("%s\n", versionInfo[i]);
	 }

    
    MQTTAsync_setTraceLevel(MQTTASYNC_TRACE_PROTOCOL);
	MQTTAsync_setTraceCallback(handleTrace);

	getopts(argc, argv);

	if (options.test_no == 0)
	{ /* run all the tests */
		for (options.test_no = 1; options.test_no < ARRAY_SIZE(tests); ++options.test_no)
			rc += tests[options.test_no](options); /* return number of failures.  0 = test succeeded */
	}
	else
		rc = tests[options.test_no](options); /* run just the selected test */

	MyLog(LOGA_INFO, "Total tests run: %d", *numtests);
	if (rc == 0)
		MyLog(LOGA_INFO, "verdict pass");
	else
        MyLog(LOGA_INFO, "verdict fail");

	return rc;
}

