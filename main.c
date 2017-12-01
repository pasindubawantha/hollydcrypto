#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

int add_meta_data(FILE * file, char * file_name);
int my_encrypt(char * file_name, char * password);
int my_decrypt(char * file_name, char * password);
int cypher(char * in, char out[255], char * password);
int check_encrypt(char * name);
int check_decrypt(char * name);
void print_help(void);

int main(int argc, char *argv[]) {
	if(argc != 4){
		print_help();
		return -1;
	}
	int fail_count =0;

	DIR *directory;
	struct dirent *dir;

	char * password = argv[3];
	directory = opendir(argv[2]);
	printf("Acessing source directory\n");

	if (directory){
		while ((dir = readdir(directory)) != NULL){
			if(argv[1][0] == 'e'){
				if(dir->d_name[0] != '.' && check_encrypt(dir->d_name)){
					printf("encypting : %s\n", dir->d_name);
					if(my_encrypt(dir->d_name, password)){
						printf("##### FAILED to encryp %s\n", dir->d_name);
						fail_count++;
					}
				}
			}else if(argv[1][0] == 'd'){
				if(check_decrypt(dir->d_name)){
					printf("decypting : %s\n", dir->d_name);
					if(my_decrypt(dir->d_name, password)){
						printf("##### FAILED to decrypt %s\n", dir->d_name);
						fail_count++;
					}
				}
			}else{
				print_help();
				return -1;
			}
		}
		closedir(directory);
		if(fail_count){
			printf("Done with %d failed operations!\n",fail_count);
		}else{
			printf("Done !\n");
		}
	}else{
		printf("Source directory error \n");
		return -1;
	}
	return 0;
}


int my_decrypt(char * file_name, char * password){
	FILE *file;
	size_t size = 4096;
	size_t number = 1;
	char old_name[255];
	int file_size;

	char * buffer;
	if(!(buffer = malloc(size*number))){
		printf("Failed to allocate memory\n");
		return -1;
	}

	if(!(file = fopen(file_name, "rb+"))){
		printf("Can't open file\n");
		return -1;
	}
	fseek( file, -(size*number), SEEK_END );

	fread(buffer, size, number, file);

	int i = size*number - 2;
	int old_name_found = 0;
	while(i >= 0){
		if(*(buffer+i) == '\0'){
			old_name_found = 1;
			break;
		}
		i--;
	}
	if(old_name_found){
		i++;
		int j=0;
		while(i<size*number){
			if(j == 255){
				printf("Old name size greater than 255 setting no name\n");
				old_name_found = 0;
				break;
			}
			old_name[j] = *(buffer+i);
			j++;
			i++;
		}
		fseek(file, 0, SEEK_END);
		file_size = ftell(file) - (j+1) ;
		old_name[j] = '\0';
	}

	fseek( file, 0, SEEK_SET );
	fread(buffer, size, number, file);
	// Decryption part
	for(int i=0; i<size*number; i++){
		*(buffer+i) = *(buffer+i) - password[i%strlen(password)];
	}

	fseek( file, 0, SEEK_SET );
	fwrite(buffer, size, number, file);
	free(buffer);

	if(!old_name_found){
		printf("Couldn't find old name\n");
		if(fclose(file)){
			printf("Failed to close file\n");
			return -1;
		}
	}else{
		if(ftruncate(fileno(file), file_size)){
			printf("Failed to srink file\n");
			return -1;
		}
		if(fclose(file)){
			printf("Fialed to close file\n");
			return -1;
		}
		if(rename(file_name, old_name)){
			printf("Failed to rename file\n");
			return -1;
		}
	}
	return 0;
}

int my_encrypt(char * file_name, char * password){
	FILE *file;
	size_t size = 4096;
	size_t number = 1;
	char new_name[255];
	if(cypher(file_name , new_name, password)){
		printf("File name is too big\n");
		return -1;
	}

	char * buffer;
	if(!(buffer = malloc(size*number))){
		printf("Failed to allocate memory\n");
		return -1;
	}
	if(rename(file_name, new_name)){
		printf("Failed to rename\n");
		return -1;
	}
	if(!(file = fopen(new_name, "rb+"))){
		printf("Can't open file\n");
		return -1;
	}

	fread(buffer, size, number, file);
	// Encryption part
	for(int i=0; i<size*number; i++){
		*(buffer+i) = *(buffer+i) + password[i%strlen(password)];
	}

	fseek( file, 0, SEEK_SET );
	fwrite(buffer, size, number, file);

	free(buffer);
	if(add_meta_data(file, file_name)){
		return -1;
	}
	return 0;
}

int add_meta_data(FILE * file, char * file_name){
	fseek( file, 0, SEEK_END);
	char delimiter[1];
	delimiter[0] = '\0';
	fwrite(delimiter, sizeof(char), 1, file);
	fwrite(file_name, strlen(file_name)*sizeof(char), 1, file);
	if(fclose(file)){
		printf("Can't close file\n");
		return -1;
	}
	return 0;
}

int check_encrypt(char * name)
{
	int i = strlen(name)-1;
	if(name[i] == 't' && name[i-1] == 'u' && name[i-2] == 'o') return 0;
	if(name[i] == 'c' && name[i-1] == '.' && name[i-2] == 'a') return 0;
	if(name[i-5]=='.' && name[i-4]=='c' && name[i-3]=='r' && name[i-2]=='y' && name[i-1]=='p' && name[i]=='t') return 0;
	return 1;
}

int check_decrypt(char * name){
	int i = strlen(name)-1;
	if(name[i-5]=='.' && name[i-4]=='c' && name[i-3]=='r' && name[i-2]=='y' && name[i-1]=='p' && name[i]=='t') return 1;
	return 0;
}

int cypher(char * in, char out[255], char * password){
	int i = 0;
	int j = strlen(in);
	if(j > 255-10){
		return -1;
	}
	while(i < strlen(in)){
		out[i] = 'a' + (in[i]+password[i%(strlen(password))])%25;
		i++;
	}
	out[i] = '.';out[i+1]='c';out[i+2]='r';out[i+3]='y';out[i+4]='p';out[i+5]='t';out[i+6]='\0';
	return 0;
}

void print_help(void){
	printf("usage: hollydcypto [e|d] source_directory password\n");
	printf("e - for encrption\n");
	printf("d - for decription\nno other direcotries in source_directory, no files with .out or .c\n");
	printf("Only .crypt files are decrypted\nFile name should be less tha 200 charchters\n");

	printf("IMPORTNAT\n");
	printf("If you decypt with wrong password, encrypt again with that password and decrypt with the right password\n");
}
