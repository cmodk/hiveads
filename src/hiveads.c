/*
 *     C ECHO client example using sockets
 *     */
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<unistd.h>
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<stdlib.h>
#include <data_logger.h>
#include <devicehive.h>
#include <debug.h>

#define ADS_REPLY_MAX (1024*1024)

int do_run;
int debug=0;
int dlmq;
int dhmq;

typedef struct {
	char icao[1024];
	char guid[1024];
} aircraft_t;

typedef enum{
	ADS_UNKNOWN,
	ADS_FLOAT,
  ADS_STRING
}ads_data_type_t;

typedef struct{
	char *stream;
	ads_data_type_t type;

  char value_str[1024];
  double value;

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
	{"flight.number",ADS_STRING},
	{"flight.altitude",ADS_FLOAT},
	{"flight.speed",ADS_FLOAT},
	{"flight.groundtrack",ADS_FLOAT},
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

int send_double_value(char *icao_number, char *stream, long long timestamp, char *value_str){

	double value;
	char device_name[1024];

	if(icao_number==NULL || strlen(icao_number)==0){
		debug_printf("Missing icao_number\n");
		return -1;
	}

	if(value_str==NULL || strlen(value_str)==0){
		debug_printf("Missing value string\n");
		return -1;
	}

	value=atof(value_str);


	sprintf(device_name,"ICAO%s",icao_number);

	return log_double(dlmq, device_name,stream,timestamp,value);

}

int send_string_value(char *icao_number, char *stream, long long timestamp, char *value_str){
	char device_name[1024];

	if(icao_number==NULL || strlen(icao_number)==0){
		debug_printf("Missing icao_number\n");
		return -1;
	}

	if(value_str==NULL || strlen(value_str)==0){
		debug_printf("Missing value string\n");
		return -1;
  }
	
	sprintf(device_name,"ICAO%s",icao_number);

  return log_string(dlmq,device_name,stream,timestamp,value_str);

}


int device_register(char *guid) {
	dhmq_device_register(dhmq,guid);
}

void trim_string(char *str, char *trimmed) {
  int i;

  memset(trimmed,0,strlen(str)+1);

  for(i=0;i<strlen;i++){
    if(str[i]==' '){
      return;
    }
    trimmed[i]=str[i];
  }
}

int main(int argc , char *argv[])
{
	aircraft_t known_aircrafts[1000];
	int num_aircrafts=0,aircraft_known;
	int sock,retval,i,j;
	struct sockaddr_in server;
	char *server_reply = (char *)malloc(sizeof(char)*ADS_REPLY_MAX);
	char **ads_data;
	int num_ads_data;
	char guid[1024];
	int ads_offset=0;
	long long timestamp;
  char trimmed[1024];

	dlmq = data_logger_mq_init();
	if(dlmq==-1){
		printf("Error opening message queue");
		return -1;
	}

	dhmq = dh_mq_init();
	if(dhmq==-1){
		printf("Error opening message queue");
		return -1;
	}

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
		memset(server_reply,0,sizeof(char) * ADS_REPLY_MAX);
		//Receive a reply from the server
		if( (retval=recv(sock , server_reply , sizeof(char)*ADS_REPLY_MAX , 0)) < 0)
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
					debug_printf("Found %s at %d\n",guid,ads_offset);

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
            case ADS_STRING:
              if(ads_data[i]==NULL)
                continue;

              trim_string(ads_data[j],trimmed);
              if(strlen(trimmed)!=0) {
                printf("%s -> %s -> '%s'\n",guid, stream_map[j].stream, trimmed);
                if(strcmp(stream_map[j].value_str,trimmed)==0){
                  print_info("Ignoring.. already sent\n");
                }else{
                  send_string_value(guid, stream_map[j].stream, timestamp, trimmed);
                  sprintf(stream_map[j].value_str,"%s",trimmed);
                }
              }
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
