/*
 *     C ECHO client example using sockets
 *     */
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<unistd.h>
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<stdlib.h>
#include <curl/curl.h>

#define debug_printf

int do_run;

typedef struct {
	char icao[1024];
	char guid[1024];
} aircraft_t;

typedef enum{
	ADS_UNKNOWN,
	ADS_FLOAT
}ads_data_type_t;

typedef struct{
	char *stream;
	ads_data_type_t type;
}ads_data_t;

ads_data_t stream_map[22]={
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"flight.altitude",ADS_FLOAT},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"loc.latitude",ADS_FLOAT},
	{"loc.longitude",ADS_FLOAT},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN},
	{"",ADS_UNKNOWN}
};

int tokenize(char *instr, char ***sepstr, char delim) {
	int nr_tokens = 0;
	char *ptr;

	if (instr != NULL) {
		*sepstr = (char **)(malloc(sizeof(char *)));
		**sepstr = instr;
		nr_tokens = 1;
		ptr = instr;
		while((ptr = strchr(ptr,delim)) != NULL) {
			*sepstr=(char **)realloc(*sepstr,(nr_tokens+1)*sizeof(char *));
			*ptr='\0';
			ptr = ptr + sizeof(char);
			(*sepstr)[nr_tokens]=(char *)ptr;
			nr_tokens++;
		}
	}
	return nr_tokens;
}

long long getTimestampMs(struct timeval* pTimeval)
{
	  struct timeval tv;

	    if(pTimeval == NULL)
		      {
			          gettimeofday(&tv,NULL);
				      pTimeval = &tv;
				        }

	      return ((((long long)pTimeval->tv_sec*1000000)+((long long)pTimeval->tv_usec))/1000);
}
void getRFC3339(long long stamp, char buf[100])
{
	struct tm nowtm;
	time_t nowtime;

	nowtime=stamp/1000;
	gmtime_r(&nowtime,&nowtm);

	char timestr[100];
	strftime(buf,100,"%Y-%m-%dT%H:%M:%S.000000Z",&nowtm);

	char miliStr[100];
	sprintf(miliStr,"%06d",(stamp%1000)*1000);
	memcpy(&(buf[20]),miliStr,6);
	//printf("Timestamp RFC: %s\n",buf);
}
			    

int post_data_priv(char *path, char *data, char *method){
	CURL *curl;
	CURLcode res;
	long http_code = 0;
	char url[2048];

	sprintf(url,"https://hive.ae101.net/%s",path);
	printf("Sending '%s' to '%s'\n",data,url);

	/* get a curl handle */ 
	curl = curl_easy_init();
	if(curl) {
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		/* First set the URL that is about to receive our POST. This URL can
		 *        just as well be a https:// URL if that is what should receive the
		 *               data. */ 
		curl_easy_setopt(curl, CURLOPT_URL, url);
		/* Now specify the POST data */ 
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "Authorization: Bearer TestToken");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER,headers);

		if(method!=NULL){
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method); /* !!! */
		}

		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
					curl_easy_strerror(res));

		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
		debug_printf("Curl res: %d\n",res);
		debug_printf("Curl Status code: %ld\n",http_code);
		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}
}

int post_data(char *path, char *data){
	return post_data_priv(path,data,NULL);
}

int put_data(char *path, char *data){
	return post_data_priv(path,data,"PUT");
}

int send_double_value(char *icao_number, char *stream, long long timestamp, char *value_str){

	double value;
	char device_name[1024];
	char url[2048];
	char data[2048];
	char timebuf[100];

	

	debug_printf("Trying to send double value\n");

	if(icao_number==NULL || strlen(icao_number)==0){
		debug_printf("Missing icao_number\n");
		return -1;
	}

	if(stream==NULL || strlen(stream)==0){
		debug_printf("Missing stream name\n");
		return -1;
	}

	if(value_str==NULL || strlen(value_str)==0){
		debug_printf("Missing value string\n");
		return -1;
	}

	value=atof(value_str);
	sprintf(device_name,"ICAO%s",icao_number);

	sprintf(url,"device/%s/notification",device_name);
	printf("Posting %f to %s as %s\n",value,device_name,stream);

	getRFC3339(timestamp,timebuf);

	sprintf(data,"{\"notification\":\"stream\",\"parameters\":{\"stream\":\"%s\",\"value\":%f,\"timestamp\":\"%s\"}}",
			stream,value,timebuf);

	post_data(url,data);


}

int device_register(char *guid) {
	char data[2048];
	char path[1024];

	sprintf(data,"{\"name\":\"%s\",\"key\":\"TestToken\",\"device_type_id\":1}",guid);
	sprintf(path,"device/%s",guid);

	put_data(path,data);

	return 0;
}


int main(int argc , char *argv[])
{
	aircraft_t known_aircrafts[1000];
	int num_aircrafts=0,aircraft_known;
	int sock,retval,i,j;
	struct sockaddr_in server;
	char message[1000] , server_reply[2000];
	char **ads_data;
	int num_ads_data;
	char guid[1024];
	int ads_offset=0;
	long long timestamp;

	curl_global_init(CURL_GLOBAL_ALL);

	//Create socket
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 30003 );

	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("connect failed. Error");
		return 1;
	}

	puts("Connected\n");

	do_run=1;
	//keep communicating with server
	while(do_run)
	{
		memset(server_reply,0,sizeof(server_reply));
		//Receive a reply from the server
		if( (retval=recv(sock , server_reply , 2000 , 0)) < 0)
		{
			printf("recv failed: %d\n",retval);
			do_run=0;
		}else if(retval==0){
			continue;
		}
		timestamp=getTimestampMs(NULL);


		debug_printf("Server reply(%d) : %s\n", retval,server_reply);
		num_ads_data=tokenize(server_reply,&ads_data,',');
		debug_printf("Num ads data: %d\n",num_ads_data);
		for(i=0;i<num_ads_data;i++){
			//printf("D: '%s'\n",ads_data[i]);
			if(strcmp(ads_data[i],"MSG")==0 || strcmp(ads_data[i],"\nMSG")==0){
					ads_offset=i;
				if(strlen(ads_data[ads_offset+4])==0){
					printf("Missing ICAO number");
					memset(guid,0,sizeof(guid));
				}else{
					ads_offset=i;
					sprintf(guid,"%s",ads_data[ads_offset+4]);
					printf("Found %s at %d\n",guid,ads_offset);

					aircraft_known=0;
					for(j=0;j<num_aircrafts;j++){
						if(strcmp(known_aircrafts[j].icao,guid) == 0){
							aircraft_known=1;
							break;
						}
					}

					if(aircraft_known==0){
						//Save and register in hive
						sprintf(known_aircrafts[num_aircrafts].icao,"%s",guid);
						sprintf(known_aircrafts[num_aircrafts].guid,"ICAO%s",guid);

						if(device_register(known_aircrafts[num_aircrafts].guid)==0){
							num_aircrafts++;
						}
					}
				}


				for(j=0;j<22;j++){
					//printf("Map: %s\n",stream_map[i].stream);
					if(strlen(stream_map[j].stream)!=0){
						debug_printf("%i: %s: %s\n",i,stream_map[j].stream, ads_data[j]);
					}else{
						debug_printf("%i: %s\n",i,ads_data[j]);
					}
					//printf("Type: %d\n",stream_map[i].type);
					switch(stream_map[j].type){
						case ADS_FLOAT:
							send_double_value(guid,stream_map[j].stream,timestamp,ads_data[j]);
							break;
					}
				}



			}
		}


	}
	printf("Application closed: %d\n",do_run);

	close(sock);
	return 0;
}
