#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<sys/stat.h> //holds variables for setting the file permissions
#include<dirent.h>
#include<time.h>
#include<regex.h>
#include<netdb.h>
#include<netinet/in.h>
#include<unistd.h>
#include<signal.h>
#include<openssl/md5.h>
#include<signal.h>
int pid, UDP_PORT;
int portno;
int udp_listsocket, listsocket, consocket, sessionkill;
struct sockaddr_in serv_addr, serv_addr2, req_serv_addr2, cli_addr;
char IP[1024];
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
//server    //client UDP
void sigh(int signo) {
	if(signo == SIGINT || signo == SIGKILL) { 
		printf("\nKILLED\n");
		FILE *fp = fopen("UdpConfig.txt", "rb");
			if(fp != NULL) { //file not deleted
				fclose(fp);
				remove("UdpConfig.txt");
			}
		kill(pid, SIGKILL);
		exit(0);
	}
}
void sigh2(int signo) {
	if(signo == SIGINT || signo == SIGKILL) { 
		printf("\nKILLED\n");
		FILE *fp = fopen("UdpConfig.txt", "rb");
			if(fp != NULL) { //file not deleted
				fclose(fp);
				remove("UdpConfig.txt");
			}
		exit(0);
	}
}
void error(char *msg) { 
	printf("%s", msg);
	exit(0);
}
void copy(FILE *f1, FILE *f2, long int num) {  //copies num bytes from f1 to f2 and f2 is overwrited
	//fseek(f1, 0, SEEK_SET); //bring file pointers to beg in f1
	//fseek(f2, 0, SEEK_SET);
	char ch; 
	//printf("\nCHARS: \n");
	while(num > 0) { 
		ch = getc(f1);
		fputc(ch, f2);
		//printf("%c", ch);
		num--;
	}
}
int parse( char *str, char substrings[10][1024]) { 
	int i, j, num, tmp;
	i = j = num = 0;

	while(str[i]!='\0' && str[i] == ' ') //skip leading white spaces
		i++;
	while(str[i]!='\0') {
		j = 0;
		tmp = i;
		while(str[i]!='\n' && str[i]!='\t' && str[i]!=' ' && str[i]!='\0') {
			j++;
			i++;
		}
		while(str[i] == '\n' || str[i] == '\t' || str[i] == ' ' && str[i]!='\0')
			i++;
		strncpy( *(substrings + num), str + tmp, j);
		*(*(substrings + num) + j) = '\0';
		num++;
	}
	return num;
}
int detectops(char *buffer, char *op, char *filename, char substrings[10][1024]) { 
	int num = parse(buffer, substrings), i;
	if( !strcmp( *(substrings), op) ) {
		int val = !strcmp(op, "FileUpload") || !strcmp(op, "FileDownload") || !strcmp(op, "FileHash"); //These Ops need a filename
		if(val)
			strcpy(filename, *(substrings + 2));
		return 1;
	}
	return 0;
}
void itoa(long long int num, char *str) { //converts integer to an array
	
	long long int n = num;
	int digits = 0, mod = 1;
	if(num == 0) {
		strcpy(str, "0");
		return;
	}
	while(n!=0)
	{
		digits++;
		mod *= 10;
		n/=10;
	}
	bzero(str, sizeof(str));
	int i = 0; 
	mod = mod/10;
	for(i = 0; i<digits; i++){ 
		str[i] = num / mod + 48;
		num %= mod;
		mod/=10;
	}
}
long int tstamp(char *str){ 
	struct tm tm;
	long int sec;
	if( sscanf(str, "%d-%d-%d %d:%d:%d", &tm.tm_mday, &tm.tm_mon, &tm.tm_year, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6) {
		tm.tm_year -= 1900;
		printf("%d\n", tm.tm_mday);
		printf("%d\n", tm.tm_mon);
		tm.tm_mon--;
		printf("%d\n", tm.tm_year);
		printf("%d\n", tm.tm_hour);
		printf("%d\n", tm.tm_min);
		printf("%d\n", tm.tm_sec);
		tm.tm_isdst = -1; 
		sec = mktime(&tm);
		printf("%ld\n", (long)sec);
	}
	return sec;
}

int match(regex_t *pexp, char *sz) {
	regmatch_t matches[1]; //A list of the matches in the string (a list of 1)
	
	if (regexec(pexp, sz, 1, matches, 0) == 0) 
		return 1;
	else 
		return 0;
	
}
int getMD5(char *filename, char *c2)
{
	unsigned char c[16];
	int i;
	FILE *inFile = fopen (filename, "rb");
	MD5_CTX mdContext;
	int bytes;
	unsigned char data[1024];

	if (inFile == NULL) {
		printf ("%s can't be opened.\n", filename);
		return 0;
	}   

	MD5_Init (&mdContext);
	while ((bytes = fread (data, 1, 1024, inFile)) != 0)
		MD5_Update (&mdContext, data, bytes);
	MD5_Final (c,&mdContext);
	for(i=0;i<16;i++)
		sprintf(&c2[i*2],"%02x", c[i]);
	fclose (inFile);
	return 0;
}

int colw[] = {15, 10, 40, 10};
void align(char *str, int colw, char *rbuffer) { 
	int len = colw - strlen(str);
	while(len > 0) {
		strcat(rbuffer, " ");
		len--;
	}
}
void sendh( char *rbuffer, FILE *fp) {
	
	bzero(rbuffer, sizeof(rbuffer));
	char str[][1024] = {
		"Name", 
		"Size", 
		"Timestamp", 
		"Type"
	};
	int i = 0;
	for(i = 0; i<4; i++) { 
		//printf("%s", *(str + i));
		signal(SIGTTOU, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		if(signal(SIGINT, sigh2) == SIG_ERR || signal(SIGQUIT, sigh2) == SIG_ERR) {
			printf("ERROR CATCHING SIGNALS\n");
			exit(0);
		}
		strcat(rbuffer, *(str + i));
		align(*(str + i), colw[i], rbuffer);
	}
	strcat(rbuffer, "\n\n");
	//printf("%s", rbuffer);
	fwrite(rbuffer, sizeof(char), strlen(rbuffer), fp);
}
void sendinfo(struct stat sb, struct dirent *ent, char *rbuffer, FILE *fp) { //displays info abt the file

	bzero(rbuffer, sizeof(rbuffer));
	int i;
	char str[4][1024];
	char str2[1024];

	strcpy(*(str), ent->d_name); //name

	itoa((long long) sb.st_size, *(str + 1)); //size	
	
	strcpy( *(str + 2), ctime(&sb.st_mtime)); //timestamp
	str[2][strlen(str[2]) - 1] = '\0';


	switch (sb.st_mode & S_IFMT) {
		case S_IFBLK:  strcpy(str2, "block device");            break;
		case S_IFCHR:  strcpy(str2, "character device");        break;
		case S_IFDIR:  strcpy(str2, "directory");               break;
		case S_IFIFO:  strcpy(str2 , "FIFO/pipe");               break;
		case S_IFLNK:  strcpy(str2, "symlink");                 break;
		case S_IFREG:  strcpy(str2,"regular file");            break;
		case S_IFSOCK: strcpy(str2, "socket");                  break;
		default:       strcpy(str2,"unknown?");                break;
	}
	strcpy(*(str + 3), str2);
	for(i = 0; i<4; i++) {
		signal(SIGTTOU, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		if(signal(SIGINT, sigh2) == SIG_ERR || signal(SIGQUIT, sigh2) == SIG_ERR) {
			printf("ERROR CATCHING SIGNALS\n");
			exit(0);
		}
		strcat(rbuffer, *(str + i));
		align(*(str + i), colw[i], rbuffer);
	}
	strcat(rbuffer, "\n");
	//printf("%s", rbuffer);
	fwrite(rbuffer, sizeof(char), strlen(rbuffer), fp);
}

int dwnldReq(int sockfd, char *filename, int protocol) { 
	//variables
	FILE *fp, *fp2;
	int bytesread, fd;
	char rbuffer[256];
	char bog[1024] = "SharedFolder/";
	char md5[32];
	char tstamp[40];
	char fsize[40];
	char buffer[1024];
	struct stat sb;
	//functionality
	strcpy(bog, "SharedFolder/");
	strcat(bog, filename);
	strcpy(filename, bog);

	getMD5(filename, md5);

	fp = fopen(md5, "wb+"); //tmp file
	fp2 = fopen(filename, "rb+"); //open orig file for reading
	stat(filename, &sb);

	fseek(fp2, 0, SEEK_END); //calculate size of original file
	long int size = ftell(fp2);
	//printf("Bytes: %ld\n", size);
	fseek(fp2, 0, SEEK_SET);
	fseek(fp, 0, SEEK_SET);
	copy(fp2, fp, size); //copy entire orig into tmp

	itoa( (long long )sb.st_size, fsize);
	strcpy(buffer, fsize);
	align(fsize, 40, buffer);

	strcat(buffer, ctime(&sb.st_mtime));
	align( ctime(&sb.st_mtime), 40, buffer);
	strcat(buffer, md5);

	fwrite(buffer, sizeof(char), 112, fp); //add md5 at the end
	fseek(fp, 0, SEEK_SET); //bring back to 0 pos
	//printf("\nContent Sent: \n");
	if( protocol == SOCK_STREAM ) {
		while( (bytesread = fread(rbuffer, sizeof(char), sizeof(rbuffer) - 1, fp))  > 0 ) //still chunks are arriving
		{
			rbuffer[bytesread] = 0;
			//printf("%s", rbuffer);
			write(sockfd, rbuffer, bytesread);
			if(bytesread != (sizeof(rbuffer) - 1 ) )
				break;
			bzero( rbuffer, sizeof(rbuffer));
		}
	}
	else if(protocol == SOCK_DGRAM) {
		
		char addr[1024];
		//printf("Data at:%d\n", cli_addr.sin_port);
		//printf("Sending To: %d.%d.%d.%d\n", (int)(cli_addr.sin_addr.s_addr&0xFF), (int)((cli_addr.sin_addr.s_addr&0xFF00)>>8), (int)((cli_addr.sin_addr.s_addr&0xFF0000)>>16), (int)((cli_addr.sin_addr.s_addr&0xFF000000)>>24));
		bind(udp_listsocket, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
		fd = fileno(fp);
		while( (bytesread = read(fd, rbuffer, sizeof(rbuffer) - 1))  > 0 ) //still chunks are arriving
		{
			rbuffer[bytesread] = 0;
			//printf("%s", rbuffer);
			//udp already init coz getMessage re directed here and cli_addr was init by myip
			if( sendto(udp_listsocket, rbuffer, bytesread, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0) {
				printf("ERROR SENDING MSG. OP: SENDTO\n");
				fclose(fp);
				return;
			}
			if(bytesread != (sizeof(rbuffer) - 1 ) )
				break;
			bzero( rbuffer, sizeof(rbuffer));
		}
	}
	//printf("OUT OF SENDING FILE LOOP\n");
	fclose(fp);
	remove(md5);
	fclose(fp2);

}
void hashInfo(char *buffer, char *filename) {
	char md5[32];
	char bog[1024] = "SharedFolder/";
	struct stat sb;

	strcat(bog, filename);
	stat(bog, &sb);
	bzero(buffer, sizeof(buffer));
	
	//filename
	strcpy(buffer, filename);
	align(filename, 40, buffer);
	
	//checksum
	getMD5(bog, md5);
	strcat(buffer, md5);
	align(md5, 40, buffer);

	//Timestamp
	strcat(buffer, ctime(&sb.st_mtime));
	strcat(buffer, "\n");
}
void hashHeader(char *buffer) {
	bzero(buffer, sizeof(buffer));

	strcpy(buffer, "Name");
	align("Name", 40, buffer);

	strcat(buffer, "Checksum");
	align("Checksum", 40, buffer);
	
	strcat(buffer, "TStamp");
	align("TStamp", 40, buffer);
	
	strcat(buffer, "\n");
}
void hashReq(int sockfd, char substrings[10][1024]) {
	char buffer[1024];
	int bytesread;
	FILE *fp = fopen("HashTmp.txt", "wb+");
	if(fp == NULL) { 
		printf("\nCOULD NOT OPEN HASHTMP\n");
		return;
	}
	hashHeader(buffer);
	fwrite(buffer, sizeof(char), strlen(buffer), fp);
	
	if( !strcmp(*(substrings + 1), "--verify") ) {
		bzero(buffer, sizeof(buffer));
		hashInfo(buffer, *(substrings + 2));
		fwrite(buffer, sizeof(char), strlen(buffer), fp);
	}
	else if( !strcmp(*(substrings + 1), "--checkall") ) {
		DIR *dir = opendir("SharedFolder/");
		struct dirent *ent;
		
		while( (ent = readdir(dir))!=NULL) { 
			hashInfo(buffer, ent->d_name);
			fwrite(buffer, sizeof(char), strlen(buffer), fp);
		}
		closedir(dir);
	}

	//sending file stream
	fseek(fp, 0, SEEK_SET);
	bzero(buffer, sizeof(buffer));
	while( (bytesread = fread(buffer, sizeof(char), sizeof(buffer) - 1, fp)) > 0) {
		buffer[bytesread] = 0;
		write(sockfd, buffer, bytesread);
		if(bytesread!=sizeof(buffer) - 1) { 
			break;
		}
		bzero(buffer, sizeof(buffer));
	}
	fclose(fp);
	if(remove("HashTmp.txt")!=0)
		printf("FILE HASHTMP DELETION ERROR\n");

}	
void uploadReq(int newfd, char *filename, int protocol) { 

	//variables
	FILE *fp;
	int bytesread, i;
	char buffer[1024];
	char rbuffer[1024];
	char bog[1024] = "SharedFolder/";
	FILE  *fpc;
	char ch;
	char fl[33];
	char md5[33];
	char choice[1024];
	int len;
	//functionality
	//printf("\n****** Receiving File from the Client ******\n");
	/* read(newfd, choice, sizeof(choice)-1); //is that side ready for response
	printf("\nUPLOAD REQUEST FOR %s\n", filename);
	scanf("%s", choice);
	printf("RESPONSE: %s\n", choice);
	write(newfd, choice, strlen(choice));
	if( !(strcmp(choice, "Deny") ) ) {
		return;
	}
	*/
	strcat(bog, filename);
	strcpy(filename, bog);

	fp = fopen(filename, "wb+"); //open file for writing file will  be created if doesnt exist
	bzero(rbuffer, sizeof(rbuffer));
	//printf("\nContent Received\n");
	if(protocol == SOCK_STREAM)
	{
		while( (bytesread = read(newfd, rbuffer, sizeof(rbuffer) - 1) )  > 0) //detecting eof 
		{
			//printf("\nBytesread : %d\n", bytesread);
			fwrite( rbuffer, sizeof(char), bytesread, fp );
			//printf("%s", rbuffer);
			if(bytesread != sizeof(rbuffer) - 1)
				break;
			bzero(rbuffer, sizeof(rbuffer));
		}
	}
	else if(protocol == SOCK_DGRAM)
	{
		//udp already init by getMessage and cli_addr init by myip
		if(udp_listsocket == 0) //socket was closed bcoz of some error 
		{
			udp_listsocket = socket(AF_INET, SOCK_DGRAM, 0);

			bzero(&serv_addr2, sizeof(serv_addr2));
			serv_addr2.sin_family = AF_INET;
			serv_addr2.sin_addr.s_addr = htonl(INADDR_ANY);
			serv_addr2.sin_port = htons(portno + 1);
			len = sizeof(serv_addr2);
		}
		if( bind(udp_listsocket, (struct sockaddr *)&serv_addr2, sizeof(serv_addr2) ) < 0)
		{
			printf("ERROR BINDIN UDP SOCKET. OP: FileUpload\n");
			fclose(fp);
			remove(filename);
			close(udp_listsocket);
			udp_listsocket = 0;
			return;
		}
		while( (bytesread = recvfrom(udp_listsocket, rbuffer, sizeof(rbuffer) -1, 0, (struct sockaddr *)&cli_addr, &len) )  > 0) //detecting eof 
		{
			//printf("\nBytesread : %d\n", bytesread);
			fwrite( rbuffer, sizeof(char), bytesread, fp );
			//printf("%s", rbuffer);
			if(bytesread != sizeof(rbuffer) - 1)
				break;
			bzero(rbuffer, sizeof(rbuffer));
		}
	}
	//printf("OUT OF LOOP\n");
	fseek(fp, -32, SEEK_END); //goto 33 character from end which is beg of md5
	for(i = 0; i<32; i++) {
		ch = getc(fp);
		fl[i] = ch;
	}
	fl[32] = '\0';
	md5[32] = '\0';
	printf("\nHash: %s\n", fl);

	printf("TStamp: ");
	fseek(fp, -40 - 32, SEEK_END);
	for(i = 0; i<40; i++) {
		ch = getc(fp);
		printf("%c", ch);
	}
	printf("\n");
	fpc = fopen(fl, "wb");
	if(fpc == NULL) {
		printf("FAILED TO OPEN FILE: %s\n", fl);
		return;
	}
	fseek(fp, 0, SEEK_END);
	long int size = ftell(fp) - 32 - 40;	
	//printf("Size: %ld\n", size);
	fseek(fp, 0, SEEK_SET);
	copy(fp, fpc, size); //copy all bytes except md5 into fpc*/
	fclose(fp);
	
	remove(filename);
	rename(fl, filename);
	
	fclose(fpc);
	getMD5(filename, md5);
	if( strcmp(md5, fl) )  {//md5 correct
		printf("FILE WAS CORRUPTED WHILE DOWNLOADING\n");
	}
	//printf("\n\n****** FILE RECEIVED ******\n\n");
}
void igReq(int newfd, char substrings[10][1024]) {

	//printf("Handling Request: IndexGet\n");
	//variables
	FILE *fp;
	char rbuffer[256];
	struct stat sb;
	char bog[1024];
	int bytesread;
	//functionality
	if( !strcmp( *(substrings + 1), "--longlist") ) {
		DIR *dir = opendir("SharedFolder/");
		struct dirent *ent;
		int num = 0;


		char filename[] = "tmp.txt";
		fp = fopen(filename, "wb+"); //erase old content

		//write col header

		bzero(rbuffer, sizeof(rbuffer));	

		sendh(rbuffer, fp); //writing 
		bzero(rbuffer, sizeof(rbuffer));	

		dir = opendir("SharedFolder/");
		while( (ent = readdir(dir))!=NULL) {

			strcpy(bog,"SharedFolder/");
			strcat(bog, ent->d_name);
			if(stat(bog, &sb) == -1) { 
				printf("ERROR obtaining info about file: %s\n", ent->d_name);
			}
			else { 
				sendinfo(sb, ent, rbuffer, fp);
			}
		}

		fseek(fp, 0, SEEK_SET);
		bzero(rbuffer, sizeof(rbuffer));	
		while( (bytesread = fread(rbuffer, sizeof(char), sizeof(rbuffer) - 1, fp))  > 0 ) //still chunks are arriving
		{
			signal(SIGTTOU, SIG_IGN);
			signal(SIGTTIN, SIG_IGN);
			if(signal(SIGINT, sigh2) == SIG_ERR || signal(SIGQUIT, sigh2) == SIG_ERR) {
				printf("ERROR CATCHING SIGNALS\n");
				exit(0);
			}
			rbuffer[bytesread] = 0;
			write(newfd, rbuffer, bytesread);
			printf("%s", rbuffer);
			if(bytesread != (sizeof(rbuffer) - 1 ) )
				break;
			bzero( rbuffer, sizeof(rbuffer));
		}
		fclose(fp);
		remove(filename);


	}
	else if( !strcmp( *(substrings + 1), "--shortlist") ) {
		char t1[1024], t2[1024];

		strcpy(t1, *(substrings + 2));
		strcat(t1, " ");
		strcat(t1, *(substrings + 3));

		strcpy(t2, *(substrings + 4));
		strcat(t2, " ");
		strcat(t2, *(substrings + 5));

		long int b1 = tstamp(t1);
		long int b2 = tstamp(t2);

		DIR *dir = opendir("SharedFolder/");
		struct dirent *ent;
		int num = 0;


		char filename[] = "tmp.txt";
		fp = fopen(filename, "wb+"); //erase old content

		//write col header

		bzero(rbuffer, sizeof(rbuffer));	

		sendh(rbuffer, fp); //writing 
		bzero(rbuffer, sizeof(rbuffer));	

		dir = opendir("SharedFolder/");
		while( (ent = readdir(dir))!=NULL) {

			strcpy(bog,"SharedFolder/");
			strcat(bog, ent->d_name);
			if(stat(bog, &sb) == -1) { 
				printf("ERROR obtaining info about file: %s\n", ent->d_name);
			}
			else { 
				if(sb.st_mtime >= b1 && sb.st_mtime <=b2)
					sendinfo(sb, ent, rbuffer, fp);
			}
		}
		fseek(fp, 0, SEEK_SET);	
		bzero(rbuffer, sizeof(rbuffer));	
		while( (bytesread = fread(rbuffer, sizeof(char), sizeof(rbuffer) - 1, fp))  > 0 ) //still chunks are arriving
		{
			rbuffer[bytesread] = 0;
			write(newfd, rbuffer, bytesread);
			printf("%s", rbuffer);
			if(bytesread != (sizeof(rbuffer) - 1 ) )
				break;
			bzero( rbuffer, sizeof(rbuffer));
		}
		fclose(fp);
		remove(filename);
	}
	else if(!strcmp(*(substrings + 1), "--regex")) {

		int rv; 
		regex_t exp; 
		rv = regcomp(&exp, *(substrings + 2), REG_EXTENDED);
		if (rv != 0) {
			printf("regcomp failed with %d\n", rv);
			return;
		}


		DIR *dir = opendir("SharedFolder/");
		struct dirent *ent;
		int num = 0;


		char filename[] = "tmp.txt";
		fp = fopen(filename, "wb+"); //erase old content

		//write col header

		bzero(rbuffer, sizeof(rbuffer));	

		sendh(rbuffer, fp); //writing 
		
		bzero(rbuffer, sizeof(rbuffer));	

		dir = opendir("SharedFolder/");
		while( (ent = readdir(dir))!=NULL) {

			strcpy(bog,"SharedFolder/");
			strcat(bog, ent->d_name);
			if(stat(bog, &sb) == -1) { 
				printf("ERROR obtaining info about file: %s\n", ent->d_name);
			}
			else { 
				if( match(&exp, ent->d_name) )
					sendinfo(sb, ent, rbuffer, fp);
			}
		}
		fseek(fp, 0, SEEK_SET);
		bzero(rbuffer, sizeof(rbuffer));	
		while( (bytesread = fread(rbuffer, sizeof(char), sizeof(rbuffer) - 1, fp))  > 0 ) //still chunks are arriving
		{
			rbuffer[bytesread] = 0;
			write(newfd, rbuffer, bytesread);
			printf("%s", rbuffer);
			if(bytesread != (sizeof(rbuffer) - 1 ) )
				break;
			bzero( rbuffer, sizeof(rbuffer));
		}
		fclose(fp);
		remove(filename);

	}
	//printf("\n\n****** Waiting for the Next Request ******\n\n");
	
}
void getMessage(int argc, int portno) {
	
	int indexget = 0, fileupload = 1, filedwnld = 2, bye = 3, buflen, connect = 4, chport = 5, filehash = 6, myip = 7, udpport = 8;
	char *ops[] = {"IndexGet", "FileUpload", "FileDownload", "Bye","connect", "chport", "FileHash", "myip", "udpport"};
	char substrings[10][1024];
	char buffer[1024];
	char filename[1024];
	int len;
	struct sockaddr_in req_serv_addr;
	
	if(listsocket == 0) { 

		//creation of the socket
		listsocket = socket(AF_INET, SOCK_STREAM, 0); 
		if(listsocket < 0)  
			error("ERROR CREATING SOCKET\n");
		printf("SOCKET CREATED SUCCESSFULLY\n");

		bzero( (char *)&serv_addr, sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //listen to any IP connection
		serv_addr.sin_port = htons(portno);

		//creation of the address complete

		if( bind(listsocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr) ) < 0 ) {
			error("ERROR BINDING SOCKET\n\n");
		}
		printf("SOCKET BINDED SUCCESSFULLY\n");

		//go to sleep until receive a request
		if( listen(listsocket, 10)  < 0 ) { 
			error("Error establishing the listening connection\n");
		}

		printf("LISTENING TO %d(TCP)\n", portno);
	}
	if (udp_listsocket == 0) {
		udp_listsocket = socket(AF_INET, SOCK_DGRAM, 0);

		bzero(&serv_addr2, sizeof(serv_addr2));
		serv_addr2.sin_family = AF_INET;
		serv_addr2.sin_addr.s_addr = htonl(INADDR_ANY);
		serv_addr2.sin_port = htons(portno + 1);
		len = sizeof(serv_addr2);
		//bind(udp_listsocket, (struct sockaddr *)&serv_addr2, sizeof(serv_addr2));
		printf("\n%d(UDP)\n", portno + 1);

	}	
	bzero( (char *)&req_serv_addr, sizeof(req_serv_addr));
	int reqlen = sizeof(req_serv_addr);
	if( (consocket = accept(listsocket, (struct sockaddr *)&req_serv_addr, &reqlen))  < 0 ) { 
		error("ERROR ACCEPTING CONNECTION\n");
	}
	//-------------------TCP socket init complete----------------------------------------
	//printf("Connected To: %s\n", inet_ntoa(req_serv_addr.sin_addr));
	mkdir("SharedFolder", S_IRUSR | S_IWUSR | S_IXUSR);

	bzero(buffer, sizeof(buffer));
	buflen = 0;
	buflen = read(consocket, buffer, sizeof(buffer) - 1);
	
	mkdir("SharedFolder", S_IRUSR | S_IWUSR | S_IXUSR);
	
	if( detectops(buffer, ops[filedwnld], filename, substrings) )  // client wants to dwnld a file
	{
		if(!strcmp(*(substrings + 1), "--TCP"))
			dwnldReq(consocket, filename, SOCK_STREAM);
		else if(!strcmp(*(substrings + 1), "--UDP"))
			dwnldReq(udp_listsocket, filename, SOCK_DGRAM);
	}
	else if( detectops(buffer, ops[fileupload], filename, substrings) ) //client requested for a dwnld
	{
		if(!strcmp(*(substrings + 1), "--TCP"))
			uploadReq(consocket, filename, SOCK_STREAM);
		else if(!strcmp(*(substrings + 1), "--UDP"))
			uploadReq(udp_listsocket, filename, SOCK_DGRAM);
	}
	else if( detectops(buffer, ops[bye], filename, substrings) ) //client wrote bye to terminate the connection
	{
		error("\n****** Client terminated the connection ******\n");
	}
	else if( detectops(buffer, ops[myip], filename, substrings) ) { //init client for other side
		bzero(&cli_addr, sizeof(cli_addr));
		cli_addr.sin_family = AF_INET;
		cli_addr.sin_addr.s_addr = inet_addr( *(substrings + 1));
		cli_addr.sin_port = htons(portno + 1);
		printf("Client IP: %s\n", *(substrings + 1));
	}
	else if( detectops(buffer, ops[indexget], filename, substrings) ) 
	{ 	
		igReq(consocket, substrings);
	}
	else if( detectops(buffer, ops[filehash], filename, substrings) ) {
		hashReq(consocket, substrings);
	}
	else if( detectops(buffer, ops[udpport], filename, substrings) ) {
		//UDP_PORT = atoi(*(substrings + 1));
		printf("CLIENT UDP PORT: %s\n", *(substrings + 1));
		FILE *fp = fopen("UdpConfig.txt", "wb");
		fwrite( *(substrings + 1), sizeof(char), strlen( *(substrings + 1)), fp);
		fclose(fp);
	}
	else {
		printf("BAD REQUEST RECEIVED\n");
	}
}

int sendIgReq(char *buffer, char *serv_ip, int portno) {

	//int pid = fork();
	//if(pid != 0)
	//	return pid;
	//variables
	//printf("Here\n");
	FILE *fp;
	char bog[1024] = "SharedFolder/";
	char rbuffer[256];
	char tmp[100];
	int bytesread, buflen;
	
	struct sockaddr_in req_serv_addr;
	int sockfd = -1;

	//functionality
	bzero( (char *)&req_serv_addr, sizeof(req_serv_addr));
	while(sockfd < 0) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
	}
	req_serv_addr.sin_family = AF_INET;
	req_serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
	req_serv_addr.sin_port = htons(portno);
	while( connect(sockfd, (struct sockaddr *)&req_serv_addr, sizeof(req_serv_addr)) < 0);
	printf("CONNECTION WITH %s ESTABLISHED\n", serv_ip);	
	if( write(sockfd, buffer, strlen(buffer) ) < 0){ //sending request to other side
		printf("ERROR SENDING MSG. OP: IndexGet\n");
		return ;
	}
	bzero(rbuffer, sizeof(rbuffer));

	while( ( bytesread = read(sockfd, rbuffer, sizeof(rbuffer) - 1) ) > 0){

		rbuffer[bytesread] = 0;	
		printf("%s", rbuffer);
		if(bytesread != (sizeof(rbuffer) - 1) )
			break;

		bzero(rbuffer, sizeof(rbuffer));
	}

	printf("\n");
	close(sockfd);
}
int sendUploadReq(char *buffer, char *serv_ip, char *filename, int portno, int protocol) { 

	int pid = fork();
	if(pid != 0)
		return pid;

	//variables
	FILE *fp, *fpc;
	char bog[1024] = "SharedFolder/";
	char rbuffer[1024];
	struct stat sb;
	//char tmp[100];
	char md5[32];
	char tstamp[40]; 
	int bytesread, buflen;

	struct sockaddr_in req_serv_addr;
	int sockfd = -1;

	//functionality
	bzero( (char *)&req_serv_addr, sizeof(req_serv_addr));
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		printf("\nERROR CREATING SOCKET. OP: sendUploadReq\n");
		exit(0);
	}

	req_serv_addr.sin_family = AF_INET;
	req_serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
	req_serv_addr.sin_port = htons(portno);
	//if(protocol == SOCK_STREAM)
	if( connect(sockfd, (struct sockaddr *)&req_serv_addr, sizeof(req_serv_addr)) < 0) { 
		printf("\nERROR CONNECTING SOCKET. OP: sendUploadReq\n");
		close(sockfd);
		exit(0);
	}

	//make it the qualified path
	strcpy(bog, "SharedFolder/");
	strcat(bog, filename);
	strcpy(filename, bog);
	stat(filename, &sb);
	//if(protocol == SOCK_STREAM)
	buflen = write( sockfd, buffer, strlen(buffer) ); //sending request to other side via TCP
	//else if(protocol == SOCK_DGRAM)
	//	buflen = sendto(sockfd, buffer, strlen(buffer) , 0, (struct sockaddr *)&req_serv_addr, sizeof(req_serv_addr)); //sending request to other side	
	if(buflen < 0) {
		printf("\nERROR SENDING MSG. OP: sendUploadReq\n");
		close(sockfd);
		exit(0) ;
	}
	

	getMD5(filename, md5);
	fp = fopen(filename, "rb+"); //open for reading, file will not be created if it doesnt exist
	if(fp ==  NULL)
	{
		printf("\nERROR OPENING THE FILE. OP: sendUploadReq\n");
		close(sockfd);
		exit(0) ;
	}

	fpc = fopen(md5, "wb+");
	if(fpc ==  NULL)
	{
		printf("\nERROR OPENING THE FILE. OP: sendUploadReq\n");
		fclose(fp);
		close(sockfd);
		exit(0) ;
	}


	//buflen = read(sockfd, buffer, sizeof(buffer)); //waiting for the permission

	if(fseek(fp, 0, SEEK_END) < 0)
	{
		printf("\nERROR USING FSEEK(1). OP: sendUploadReq\n");
		fclose(fp);
		fclose(fpc);
		close(sockfd);
		exit(0);
	}

	long int size = ftell(fp);
	
	if(fseek(fpc, 0, SEEK_SET) < 0)
	{
		printf("\nERROR USING FSEEK(2). OP: sendUploadReq\n");
		fclose(fp);
		fclose(fpc);
		close(sockfd);
		exit(0);
	}

	if(fseek(fp, 0, SEEK_SET) < 0)
	{
		printf("\nERROR USING FSEEK(3). OP: sendUploadReq\n");
		fclose(fp);
		fclose(fpc);
		close(sockfd);
		exit(0);
	}

	copy(fp, fpc, size);

	strcpy(rbuffer, ctime(&sb.st_mtime));
	align(ctime(&sb.st_mtime), 40, rbuffer);

	strcat(rbuffer, md5);
	fwrite(rbuffer, sizeof(char), 40 + 32, fpc);
	if(ferror(fp)) { 
		printf("\nERROR USING FILE STREAM. OP: sendUploadReq\n");
		fclose(fp);
		fclose(fpc);
		close(sockfd);
		exit(0);
	}

	if(fseek(fpc, 0, SEEK_SET) < 0)
	{
		printf("\nERROR USING FSEEK(4). OP: sendUploadReq\n");
		fclose(fp);
		fclose(fpc);
		close(sockfd);
		exit(0);
	}

	//printf("\nContent Sent\n");
	if(protocol == SOCK_DGRAM)
	{
		FILE *file1 = fopen("UdpConfig.txt", "rb");
		if(file1 == NULL)  {
			printf("\nERRROR OPENING FILE(3). OP: sendUploadReq\n");
			fclose(fp);
			fclose(fpc);
			close(sockfd);
			exit(0);
		}
		int linelen = 0;
		char *line;
		getline(&line, &linelen, file1);
		fclose(file1);	
		//remove("UdpConfig.txt");
		cli_addr.sin_addr.s_addr = inet_addr(serv_ip);
		cli_addr.sin_port = htons(UDP_PORT = atoi(line));
		printf("UDP PORT: %d\n", UDP_PORT);
		udp_listsocket = socket(AF_INET, SOCK_DGRAM, 0); //socket at client side
		if(udp_listsocket < 0)
		{
			printf("\nERROR CREATING SOCKET. OP: sendUploadReq(UDP)\n");
			fclose(fp);
			fclose(fpc);
			close(sockfd);
			exit(0);
		}
		while( ( bytesread = fread(rbuffer, sizeof(char), sizeof(rbuffer) - 1, fpc) ) > 0) //still getting data from server
		{
			signal(SIGTTOU, SIG_IGN);
			signal(SIGTTIN, SIG_IGN);
			if(signal(SIGINT, sigh2) == SIG_ERR || signal(SIGQUIT, sigh2) == SIG_ERR) {
				printf("\nERROR CATCHING SIGNALS\n");
				fclose(fp);
				fclose(fpc);
				close(sockfd);
				close(udp_listsocket);
				exit(0);
			}
			rbuffer[bytesread] = '\0';
			//printf("%s", rbuffer);
			if (sendto(udp_listsocket, rbuffer, strlen(rbuffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0) {
				printf("\nERROR USING SENDTO. OP: sendUploadReq\n");
				fclose(fp);
				fclose(fpc);
				close(sockfd);
				close(udp_listsocket);
				exit(0);
			}
			bzero(rbuffer, sizeof(rbuffer));
		}
		close(udp_listsocket);
	}
	else if(protocol == SOCK_STREAM)
	{
		while( ( bytesread = fread(rbuffer, sizeof(char), sizeof(rbuffer) - 1, fpc) ) > 0) //still getting data from server
		{
			signal(SIGTTOU, SIG_IGN);
			signal(SIGTTIN, SIG_IGN);
			if(signal(SIGINT, sigh2) == SIG_ERR || signal(SIGQUIT, sigh2) == SIG_ERR) {
				printf("ERROR CATCHING SIGNALS. OP: sendUploadReq\n");
				fclose(fp);
				fclose(fpc);
				close(sockfd);
				exit(0);
			}
			rbuffer[bytesread] = '\0';
			//printf("%s", rbuffer);
			if (write(sockfd, rbuffer, strlen(rbuffer)) < 0) {
				
				printf("\nERROR USING WRITE. OP: sendUploadReq\n");
				fclose(fp);
				fclose(fpc);
				close(sockfd);
				exit(0);
			}
			bzero(rbuffer, sizeof(rbuffer));
		}
	}
	fclose(fp);
	fclose(fpc);
	if(remove(md5)!=0) { 
		close(sockfd);
		exit(0);
		exit(0);
	}
	close(sockfd);
	exit(0);
}
int sendDwnldReq(char *buffer, char *serv_ip, char *filename, int portno, int protocol) { 

	int pid = fork();
	if(pid != 0)
		return pid;
	//variables
	FILE *fp;
	FILE *fpc;
	char bog[1024] = "SharedFolder/";
	char rbuffer[1024];
	char rubuffer[256];
	char tmp[100];
	char fl[33];
	char md5[33];
	//char tstamp[40];
	//char fsize[40];
	int i, len, fd;
	char ch;
	int bytesread, buflen, filelen;

	struct sockaddr_in req_serv_addr, cli_addr;
	int sockfd = -1;

	//functionality
	bzero( (char *)&req_serv_addr, sizeof(req_serv_addr));
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		printf("\nERROR CREATING SOCKET. OP: myip\n");
		exit(0);
	}
	req_serv_addr.sin_family = AF_INET;
	req_serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
	req_serv_addr.sin_port = htons(portno);
	//if(protocol == SOCK_STREAM)
	if( connect(sockfd, (struct sockaddr *)&req_serv_addr, sizeof(req_serv_addr)) < 0) { 
		printf("\nERROR CONNECTING SOCKET. OP: udpport\n");
		close(sockfd);
		exit(0);
	}
	strcpy(bog, "SharedFolder/");
	strcat(bog, filename);
	strcpy(filename, bog);
	fp = fopen(filename, "wb+"); //file opened for writing
	if(fp ==  NULL)
	{
		printf("\nERROR OPENING THE FILE. OP: sendDwnldReq\n");
		close(sockfd);
		exit(0) ;
	}
	//if(protocol == SOCK_STREAM)
	buflen = write(sockfd, buffer, strlen(buffer)); //sending request to other side via TCP
	//else if(protocol == SOCK_DGRAM)
		//buflen = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&req_serv_addr, sizeof(req_serv_addr));
	if(buflen < 0)
	{
		printf("ERROR SENDING MSG.OP: sendDwnldReq\n");
		fclose(fp);
		close(sockfd);
		exit(0) ;
	}
	//printf("\nContent Received: \n");
	bzero(rbuffer, sizeof(rbuffer));
	if(protocol == SOCK_STREAM)
	{
		while( ( bytesread = read(sockfd, rbuffer, sizeof(rbuffer) - 1) ) > 0){

		//printf("%s", rbuffer);
			signal(SIGTTOU, SIG_IGN);
			signal(SIGTTIN, SIG_IGN);
			if(signal(SIGINT, sigh2) == SIG_ERR || signal(SIGQUIT, sigh2) == SIG_ERR) {
				printf("\nERROR CATCHING SIGNALS\n");
				fclose(fp);
				close(sockfd);
				exit(0);
			}
			fwrite(rbuffer, sizeof(char), bytesread, fp);
			if(bytesread != (sizeof(rbuffer) - 1) )
				break;
			bzero(rbuffer, sizeof(rbuffer));
		}
	}
	else if(protocol == SOCK_DGRAM)
	{
		//getting the por
		FILE *file1 = fopen("UdpConfig.txt", "rb");
		if(file1 == NULL)
		{
			printf("\nERROR OPENING FILE. OP: sendDwnldReq(UDP)\n");
			fclose(fp);
			close(sockfd);
			exit(0);
		}
		int linelen = 0;
		struct sockaddr_in bogus;
		char *line;
		getline(&line, &linelen, file1);
		fclose(file1);	
		char addr[1024];
		bzero(&cli_addr, sizeof(cli_addr));
		cli_addr.sin_family = AF_INET;
		cli_addr.sin_addr.s_addr = inet_addr(serv_ip);	
		cli_addr.sin_port = htons(UDP_PORT = atoi(line));
		//printf("Diff: %d\n", UDP_PORT - 9003);
		//printf("Receving at: %d\n", cli_addr.sin_port);
		//printf("Receving from: %d.%d.%d.%d\n", (int)(cli_addr.sin_addr.s_addr&0xFF), (int)((cli_addr.sin_addr.s_addr&0xFF00)>>8), (int)((cli_addr.sin_addr.s_addr&0xFF0000)>>16), (int)((cli_addr.sin_addr.s_addr&0xFF000000)>>24));
		
		udp_listsocket = socket(AF_INET, SOCK_DGRAM, 0); //socket at client side
		if(udp_listsocket < 0)
		{
			printf("\nERROR CREATING SOCKET. OP: sendDwnldReq(UDP)\n");
			fclose(fp);
			close(sockfd);
			exit(0);
		}
		if( bind(udp_listsocket, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0){
			printf("\nERROR IN BINDING UDP SOCKET. OP: FileDownload\n");
			fclose(fp);
			close(sockfd);
			close(udp_listsocket);
			exit(0);
		}
		len = sizeof(cli_addr);
		fd = fileno(fp);
		bzero(rbuffer, sizeof(rbuffer));
		while( ( bytesread = recvfrom(udp_listsocket, rubuffer, sizeof(rubuffer) - 1, 0, (struct sockaddr *)&cli_addr, &len) ) > 0){

			//printf("%s", rubuffer);
			signal(SIGTTOU, SIG_IGN);
			signal(SIGTTIN, SIG_IGN);
			if(signal(SIGINT, sigh2) == SIG_ERR || signal(SIGQUIT, sigh2) == SIG_ERR) {
				printf("ERROR CATCHING SIGNALS\n");
				fclose(fp);
				close(sockfd);
				close(udp_listsocket);
				exit(0);
			}
			write(fd, rubuffer, bytesread);
			if(bytesread != (sizeof(rubuffer) - 1) )
				break;
			bzero(rubuffer, sizeof(rubuffer));
		}
		close(udp_listsocket);
	}
	if(ferror(fp)) { 
		printf("\nERROR USING FILE STREAM. OP: sendDwnldReq\n");
		fclose(fp);
		close(sockfd);
		close(udp_listsocket);
		exit(0);
	}
	//printf("Here\n");
	if(fseek(fp, -32, SEEK_END) < 0) //goto 33 character from end which is beg of md5
	{
		printf("\nERROR USING FSEEK(1). OP: sendDwnldReq\n");
		fclose(fp);
		close(sockfd);
		close(udp_listsocket);
		exit(0);
	}
	for(i = 0; i<32; i++) { 
		ch = getc(fp);
		fl[i] = ch;
	}
	printf("\nName: %s\n", filename);
	printf("Hash: %s\n", fl);
	printf("TStamp: ");
	if(fseek(fp, -40 - 32, SEEK_END) < 0)
	{
		printf("\nERROR USING FSEEK(2). OP: sendDwnldReq\n");
		fclose(fp);
		close(sockfd);
		close(udp_listsocket);
		exit(0);
	}
	for(i = 0; i<40; i++) { 
		ch = getc(fp);
		printf("%c", ch);
	}
	printf("\n");
	printf("Size: ");
	if(fseek(fp, -40-40-32, SEEK_END) < 0)
	{
		printf("\nERROR USING FSEEK(3). OP: sendDwnldReq\n");
		fclose(fp);
		close(sockfd);
		close(udp_listsocket);
		exit(0);
	}
	for(i = 0; i<40; i++) {
		ch = getc(fp);
		printf("%c", ch);
	}
	printf("\n");
	fpc = fopen(fl, "wb");
	if(fpc == NULL)
	{
		printf("\nERROR OPENING FILE(2). OP: sendDwnldReq\n");
		fclose(fp);
		close(sockfd);
		close(udp_listsocket);
		exit(0);
	}
	if(fseek(fp, 0, SEEK_END) < 0)
	{
		printf("\nERROR USING FSEEK(4). OP: sendDwnldReq\n");
		fclose(fp);
		fclose(fpc);
		close(sockfd);
		close(udp_listsocket);
		exit(0);
	}
	long int size = ftell(fp) - (32 + 40 + 40);
	//printf("Size: %ld\n", size);
	//scanf("%*c");
	fflush(fp);
	if(fseek(fp, 0, SEEK_SET) < 0)
	{
		printf("\nERROR USING FSEEK(5). OP: sendDwnldReq\n");
		fclose(fp);
		fclose(fpc);
		close(sockfd);
		close(udp_listsocket);
		exit(0);
	}
	copy(fp, fpc, size); //copy all bytes except md5 into fpc*/
	fclose(fp);
	fclose(fpc);
	if(remove(filename)!=0)
		printf("\nERROR REMOVING FILE\n");
	if(rename(fl, filename) != 0)
		printf("\nERROR RENAMING FILE\n");
	
	getMD5(filename, md5);
	fl[32] = md5[32] = '\0';
	if( strcmp(md5, fl) )  {//md5 correct
		printf("\nFILE WAS CORRUPTED WHILE DOWNLOADING\n");
	}
	close(sockfd);
	//printf("\n\n****** File Received ******\n\n");
	exit(0);
}
void sendHashReq(char *buffer, char *serv_ip, char substrings[10][1024], int portno) {

	char rbuffer[1024];
	int bytesread;
	struct sockaddr_in req_serv_addr, cli_addr;
	int sockfd = -1;

	//functionality
	bzero( (char *)&req_serv_addr, sizeof(req_serv_addr));
	while(sockfd < 0) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
	}
	req_serv_addr.sin_family = AF_INET;
	req_serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
	req_serv_addr.sin_port = htons(portno);
	while( connect(sockfd, (struct sockaddr *)&req_serv_addr, sizeof(req_serv_addr)) < 0);
	
	if( write(sockfd, buffer, strlen(buffer) ) < 0){ //sending request to other side
		printf("ERROR SENDING MSG. OP: FileHash\n");
		return ;
	}
	while( ( bytesread = read(sockfd, rbuffer, sizeof(rbuffer) - 1) ) > 0){

		printf("%s", rbuffer);
		signal(SIGTTOU, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		if(signal(SIGINT, sigh2) == SIG_ERR || signal(SIGQUIT, sigh2) == SIG_ERR) {
			printf("ERROR CATCHING SIGNALS\n");
			exit(0);
		}
		if(bytesread != (sizeof(rbuffer) - 1) )
			break;
		bzero(rbuffer, sizeof(rbuffer));
	}
	printf("Complete\n");

}
void sendByeReq(char *buffer, char *serv_ip, int portno) { 
	
	struct sockaddr_in req_serv_addr;
	int sockfd = -1;

	//functionality
	bzero( (char *)&req_serv_addr, sizeof(req_serv_addr));
	while(sockfd < 0) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
	}
	req_serv_addr.sin_family = AF_INET;
	req_serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
	req_serv_addr.sin_port = htons(portno);
	while( connect(sockfd, (struct sockaddr *)&req_serv_addr, sizeof(req_serv_addr)) < 0);
	printf("CONNECTION WITH %s ESTABLISHED\n", serv_ip);	
	if( write(sockfd, buffer, strlen(buffer) ) < 0){ //sending request to other side
		printf("ERROR SENDING MSG. OP: IndexGet\n");
		return ;
	}
}
void sendMeReq(char *buffer, char *serv_ip, int portno) { 
	
	struct sockaddr_in req_serv_addr;
	int sockfd = -1;

	//functionality
	bzero( (char *)&req_serv_addr, sizeof(req_serv_addr));
	//while(sockfd < 0) {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	//}
	if(sockfd < 0) {
		printf("\nERROR CREATING SOCKET. OP: myip\n");
		return;
	}
	
	req_serv_addr.sin_family = AF_INET;
	req_serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
	req_serv_addr.sin_port = htons(portno);
	if( connect(sockfd, (struct sockaddr *)&req_serv_addr, sizeof(req_serv_addr)) < 0) { 
		printf("\nERROR CONNECTING SOCKET. OP: udpport\n");
		close(sockfd);
		return;
	}
	printf("CONNECTION WITH %s ESTABLISHED\n", serv_ip);	
	if( write(sockfd, buffer, strlen(buffer) ) < 0){ //sending request to other side
		printf("ERROR SENDING MSG. OP: IndexGet\n");
		close(sockfd);
		return ;
	}
}
void sendUdpportReq(char *buffer, char *serv_ip, int portno) { 
	
	struct sockaddr_in req_serv_addr;
	int sockfd = -1;

	//functionality
	bzero( (char *)&req_serv_addr, sizeof(req_serv_addr));
	//if(sockfd < 0) {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		printf("\nERROR CREATING SOCKET. OP: udpport\n");
		return;
	}
	//}
	req_serv_addr.sin_family = AF_INET;
	req_serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
	req_serv_addr.sin_port = htons(portno);
	if( connect(sockfd, (struct sockaddr *)&req_serv_addr, sizeof(req_serv_addr)) < 0) { 
		printf("\nERROR CONNECTING SOCKET. OP: udpport\n");
		close(sockfd);
		return;
	}
	printf("\nCONNECTION WITH %s ESTABLISHED\n", serv_ip);	
	if( write(sockfd, buffer, strlen(buffer) ) < 0){ //sending request to other side
		printf("\nERROR SENDING MSG. OP: udpport\n");
		close(sockfd);
		return ;
	}
}

int main(int argc, char *argv[])
{
	char buffer[1024];
	char filename[1024];
	char substrings[10][1024];
	int indexget = 0, fileupload = 1, filedwnld = 2, bye = 3, buflen, connect = 4, chport = 5, filehash = 6, myip = 7, udpport = 8;
	char *ops[] = {"IndexGet", "FileUpload", "FileDownload", "Bye","connect", "chport", "FileHash", "myip", "udpport"};
	if(argc == 2) //server set up 
	{
		portno = atoi(argv[1]);
		pid = fork();
	}
	else if(argc == 3)
		portno = atoi(argv[2]);
	
	while(pid == 0) {
		getMessage(argc, portno); //getting messages via child process
	}
	while(1) {
		signal(SIGTTOU, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		if(signal(SIGINT, sigh) == SIG_ERR || signal(SIGQUIT, sigh) == SIG_ERR) {
			printf("ERROR CATCHING SIGNALS\n");
			FILE *fp = fopen("UdpConfig.txt", "rb");
			if(fp != NULL) { //file not deleted
				fclose(fp);
				remove("UdpConfig.txt");
			}
			exit(0);
		}
		bzero(buffer, sizeof(buffer));
		strcpy(buffer, "\n");
		while( !strcmp(buffer, "\n")) { 
			bzero(buffer, sizeof(buffer));
			printf("$: ");
			fgets(buffer, sizeof(buffer) - 1, stdin);
		}
		buffer[strlen(buffer) - 1] = '\0';
		if( detectops(buffer, ops[bye], filename, substrings) )//client wrote bye, terminate the connection
		{
			sendByeReq(buffer, IP, portno);
			FILE *fp = fopen("UdpConfig.txt", "rb");
			if(fp != NULL) { //file not deleted
				fclose(fp);
				remove("UdpConfig.txt");
			}
			close(listsocket);
			signal(SIGQUIT, SIG_IGN);
			kill(pid, SIGQUIT);
			printf("\n****** Connection with the client terminated ******\n");
			break;
		}
		else if( detectops(buffer, ops[filedwnld], filename, substrings)) //Request to dwnld a file
		{
			if(!strcmp(*(substrings + 1), "--TCP"))
				sendDwnldReq(buffer, IP, filename, portno, SOCK_STREAM);
			else if(!strcmp(*(substrings + 1), "--UDP")) {
				sendDwnldReq(buffer, IP, filename, portno, SOCK_DGRAM);
			}
			else {
				printf("PROTOCOL FLAG INVALID\n");
			}
		}
		else if( detectops(buffer, ops[fileupload], filename, substrings) )
		{		
			int cpid;
			if(!strcmp(*(substrings + 1), "--TCP"))
				cpid = sendUploadReq(buffer, IP, filename, portno, SOCK_STREAM);
			else if(!strcmp(*(substrings + 1), "--UDP"))
				cpid = sendUploadReq(buffer, IP, filename, portno, SOCK_DGRAM);
			else {
				printf("PROTOCOL FLAG INVALID\n");
			}
			while( cpid != waitpid(cpid, NULL, 0));
		}
		else if( detectops(buffer, ops[indexget], filename, substrings) )
		{ 
			if( !strcmp( *(substrings + 1), "--longlist") || !strcmp( *(substrings + 1), "--shortlist") || !strcmp(*(substrings + 1), "--regex") )
			{
				sendIgReq(buffer, IP, portno);
			}
			else {
				printf("2ND FLAG INVALID\n");
			}
		}
		else if( detectops(buffer, ops[connect], filename, substrings)) {
			strcpy(IP, *(substrings + 1));
			printf("Request to connect to %s\n", IP);
			
			//client to send msg to init
			cli_addr.sin_family = AF_INET;
			cli_addr.sin_addr.s_addr = inet_addr(*(substrings + 1));
			printf("Receving from: %d.%d.%d.%d\n", (int)(cli_addr.sin_addr.s_addr&0xFF), (int)((cli_addr.sin_addr.s_addr&0xFF00)>>8), (int)((cli_addr.sin_addr.s_addr&0xFF0000)>>16), (int)((cli_addr.sin_addr.s_addr&0xFF000000)>>24));
		}
		else if( detectops(buffer, ops[chport], filename, substrings ) ) {
			portno = atoi(*(substrings + 1));
			printf("Requests will be sent to %d\n", portno);
		}
		else if( detectops(buffer, ops[filehash], filename, substrings) ) {
			if( !strcmp( *(substrings +  1), "--checkall") || !strcmp( *(substrings +  1), "--verify") )
				sendHashReq(buffer, IP, substrings, portno);
			else {
				printf("2ND FLAG INVALID\n");
			}
		}
		else if( detectops(buffer, ops[myip], filename, substrings) ) {
			sendMeReq(buffer, IP, portno);
		}
		else if( detectops(buffer, ops[udpport], filename, substrings)) { //telling other side about your UDP PORT
			sendUdpportReq(buffer, IP, portno);
		}
		else { 
			printf("\n\nBAD REQUEST\n\n");
		}

	}
	return 0;
}
