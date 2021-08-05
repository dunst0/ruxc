/**
 * build on macos:
 *   cd ..; gcc -o examples/httpcli -I include/ examples/httpcli.c target/release/libruxc.a -framework Security
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <ruxc.h>

static char *version = "httpcli 0.1.0";
static char* helpmsg = "\
Usage: httpcli [-D] [-p] [-t timeout] [-n count]\n\
Options:\n\
    -D            switch off debug mode\n\
    -n count      number of requests to be sent\n\
    -p            do http post instead of get\n\
    -r mode       reuse mode\n\
    -t usec       microseconds timeout\n\
    -h            this help message\n\
";

int main(int argc, char *argv[])
{
	RuxcHTTPRequest v_http_request = {0};
	RuxcHTTPResponse v_http_response = {0};
	char *url_list[] = {
		"http://www.kamailio.org/pub/kamailio/latest-stable-version-number",
		"https://www.kamailio.org/pub/kamailio/latest-stable-version-number",
		"http://www.kamailio.org/pub/kamailio/latest/README",
		"https://www.kamailio.org/pub/kamailio/latest/README",
		"http://www.kamailio.org/pub/kamailio/5.5.0/README",
		"https://www.kamailio.org/pub/kamailio/5.5.0/README",
		NULL
	};
	int post = 0;
	char c = 0;
	int ncount = 1000;
	int i = 0;
	char hdrbuf[256];
	struct timeval tvb = {0}, tve = {0};
	unsigned int diff = 0;
	int debug = 1;
	int timeout = 5000;
	char *postdata = "{ \"info\": \"testing\", \"id\": 80 }";
	int reuse = 0;

	opterr=0;
	while ((c=getopt(argc,argv, "n:r:t:Dhp"))!=-1){
		switch(c){
			case 'n':
				ncount = atoi(optarg);
				if(ncount<=0) { ncount = 1000; }
				break;
			case 'r':
				reuse = atoi(optarg);
				if(reuse<0 || reuse>2) { reuse = 0; }
				break;
			case 'D':
				debug = 0;
				break;
			case 'p':
				post = 1;
				break;
			case 't':
				timeout = atoi(optarg);
				if(timeout<=0) { timeout = 5000; }
				break;
			case 'h':
				printf("version: %s\n", version);
				printf("%s", helpmsg);
				exit(0);
				break;
			default:
				printf("unknown cli option %c\n", c);
				exit(-1);
		}
	}

	v_http_request.timeout = timeout;
	v_http_request.timeout_connect = timeout;
	v_http_request.timeout_read = timeout;
	v_http_request.timeout_write = timeout;
	v_http_request.debug = debug;
	v_http_request.reuse = reuse;

	for(i = 0; url_list[i]!=NULL; i++) {
		printf("\n* c:: request %d =========================\n\n", i);
		v_http_request.url = url_list[i];
		v_http_request.url_len = strlen(v_http_request.url);

		snprintf(hdrbuf, 255, "X-My-Key: KEY-%d\r\nX-Info: REQUEST-%d\r\n", i, i);
		v_http_request.headers = hdrbuf;
		v_http_request.headers_len = strlen(v_http_request.headers);

		if(post==1) {
			v_http_request.data = postdata;
			v_http_request.data_len = strlen(v_http_request.data);
			gettimeofday(&tvb, NULL);
			ruxc_http_post(&v_http_request, &v_http_response);
			gettimeofday(&tve, NULL);
		} else {
			gettimeofday(&tvb, NULL);
			ruxc_http_get(&v_http_request, &v_http_response);
			gettimeofday(&tve, NULL);
		}
		diff = (tve.tv_sec - tvb.tv_sec) * 1000000 + (tve.tv_usec - tvb.tv_usec);
		printf("* c:: http request[%d] done in: %u usec\n", i, diff);


		if(v_http_response.retcode < 0) {
			printf("* c:: failed to perform http get [%d] - retcode: %d\n", i, v_http_response.retcode);
		} else {
			if(v_http_response.resdata != NULL &&  v_http_response.resdata_len>0) {
				printf("* c:: response [%d] code: %d - data len: %d - data: [%.*s]\n", i,
						v_http_response.rescode, v_http_response.resdata_len,
						v_http_response.resdata_len, v_http_response.resdata);
			}
		}
		ruxc_http_response_release(&v_http_response);

		v_http_response.resdata = NULL;
		v_http_response.resdata_len = 0;
		v_http_response.retcode = 0;
		v_http_response.rescode = 0;

		if(ncount>0 && i>=ncount-1) {
			break;
		}
	}
	printf("\n");

	return 0;
}
