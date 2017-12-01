# hollydcrypto
.h>
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
	fwrite(file_name, strlen(file_name)*size