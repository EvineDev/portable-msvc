#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define BUFFER_SIZE 4096

SECURITY_ATTRIBUTES default_dir_security_attributes = {
	sizeof(SECURITY_ATTRIBUTES), 0, true,
};

char *vcvarsportable =
"@echo off\r\n"
"set PATH=%CD%;%PATH%\r\n"
"set INCLUDE=%CD%\\include\r\n"
"set LIB=%CD%\\lib\r\n"
"set LIBPATH=%CD%\\libpath\r\n";


void copy_cl_files(char *sub_dir, char *str, int len) {
	char *buffer1 = malloc(BUFFER_SIZE);
	char *buffer2 = malloc(BUFFER_SIZE);

	snprintf(buffer1, BUFFER_SIZE, "%.*s\\%s*", len, str, sub_dir);
	//printf("DIR: %s\n", buffer1);

	WIN32_FIND_DATAA find_data = {0};
	HANDLE find_handle = FindFirstFileA(buffer1, &find_data);

	if(find_handle == INVALID_HANDLE_VALUE) {
		if(GetLastError() == ERROR_PATH_NOT_FOUND) {
			return;
		}
		printf("Error: %u\n", GetLastError());
		assert(0);
	}

	BOOL ok;
	do {
		if((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			snprintf(buffer1, BUFFER_SIZE, "%.*s\\%s%s", len, str, sub_dir, find_data.cFileName);
			snprintf(buffer2, BUFFER_SIZE, "msvc\\%s%s", sub_dir, find_data.cFileName);

			BOOL success = CopyFile(buffer1, buffer2, true);
			if(success == (BOOL)false) {
				DWORD last_error = GetLastError();
				if(last_error == ERROR_FILE_EXISTS) {
					printf("File \"%s\" already exist\n", find_data.cFileName);
				} else {
					printf("Error: %u, File: %s\n", last_error, buffer2);
					assert(0);
				}
			}
		} else {
			if(strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
				snprintf(buffer2, BUFFER_SIZE, "msvc\\%s%s", sub_dir, find_data.cFileName);
				BOOL dir_ok = CreateDirectoryA(buffer2, &default_dir_security_attributes);
				if(dir_ok == false) {
					DWORD dir_last_error = GetLastError();
					if(dir_last_error != ERROR_ALREADY_EXISTS) {
						printf("Error: %u\n", dir_last_error);
						assert(0);
					}
				}

				snprintf(buffer1, BUFFER_SIZE, "%s%s\\", sub_dir, find_data.cFileName);
				copy_cl_files(buffer1, str, len);
			}
		}

		ok = FindNextFileA(find_handle, &find_data);
	} while(ok);

	assert(GetLastError() == ERROR_NO_MORE_FILES);

	free(buffer2);
	free(buffer1);
	FindClose(find_handle);
}

void copy_msvc_files(char *kind, char *str, int len, bool *contin) {
	char buffer1[BUFFER_SIZE] = {0};
	char buffer2[BUFFER_SIZE] = {0};

	if(len <= 0) {
		return;
	}
	printf("%.*s\n", len, str);

	snprintf(buffer1, BUFFER_SIZE, "%.*s\\*", len, str);
	WIN32_FIND_DATAA find_data = {0};
	HANDLE find_handle = FindFirstFileA(buffer1, &find_data);

	if(find_handle == INVALID_HANDLE_VALUE) {
		if(GetLastError() == ERROR_PATH_NOT_FOUND) {
			return;
		}
		printf("Error: %u\n", GetLastError());
		assert(0);
	}


	if(strcmp(kind, "path") == 0) {
		BOOL dir_ok = CreateDirectoryA("msvc", &default_dir_security_attributes);
		if(dir_ok == false) {
			DWORD dir_last_error = GetLastError();
			if(dir_last_error != ERROR_ALREADY_EXISTS) {
				printf("Error: %u\n", dir_last_error);
				assert(0);
			}
		}
	} else {
		snprintf(buffer2, BUFFER_SIZE, "msvc\\%s", kind);
		BOOL dir_ok = CreateDirectoryA(buffer2, &default_dir_security_attributes);
		if(dir_ok == false) {
			DWORD dir_last_error = GetLastError();
			if(dir_last_error != ERROR_ALREADY_EXISTS) {
				printf("Error: %u\n", dir_last_error);
				assert(0);
			}
		}
	}


	BOOL ok;
	do {
		if((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			//printf("- %s\n", find_data.cFileName);
			if(strcmp(kind, "path") == 0) {
				if(strcmp(find_data.cFileName, "cl.exe") == 0) {
					printf("found cl: %.*s\\%s\n", len, str, find_data.cFileName);
					copy_cl_files("", str, len);
					*contin = false;
					goto finished;
				}
			} else {
				snprintf(buffer1, BUFFER_SIZE, "%.*s\\%s", len, str, find_data.cFileName);
				snprintf(buffer2, BUFFER_SIZE, "msvc\\%s\\%s", kind, find_data.cFileName);
				BOOL success = CopyFile(buffer1, buffer2, (BOOL)true);
				if(success == (BOOL)false) {
					DWORD last_error = GetLastError();
					if(last_error == ERROR_FILE_EXISTS) {
						printf("File \"%s\" already exist\n", find_data.cFileName);
					} else {
						printf("Error: %u\n", last_error);
						assert(0);
					}
				}
			}
		}

		ok = FindNextFileA(find_handle, &find_data);
	} while(ok);

	assert(kind != "path" && "cl.exe was not found in PATH environment variable");
	assert(GetLastError() == ERROR_NO_MORE_FILES);

	finished:;

	FindClose(find_handle);
}

void process_env_path(char *env) {
	printf("-- %s\n", env);
	char path[BUFFER_SIZE];
	DWORD path_len = GetEnvironmentVariable(env, path, sizeof(path));
	assert(path_len > 0);
	assert(path_len < sizeof(path));

	bool contin = true;
	int j = 0;
	for(int i = 0; i < path_len; ++i) {
		if(path[i] == ';') {
			copy_msvc_files(env, path + j, i - j, &contin);
			if(contin == false) {
				goto finished;
			}
			j = i + 1;
		}
	}
	// Note: Get the last iteration as well
	copy_msvc_files(env, path + j, path_len - j, &contin);

	finished:;
	//printf("--\n%s\n", path);
}

void write_vcvarsportable() {
	int file_length = strlen(vcvarsportable);

	HANDLE vc_file = CreateFileA("msvc/vcvarsportable.bat", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	DWORD vc_file_written = 0;
	BOOL ok = WriteFile(vc_file, vcvarsportable, file_length, &vc_file_written, 0);
	if(ok == false) {
		printf("%u\n", GetLastError());
		assert(0);
	}
	assert(file_length == vc_file_written);
}


void generate() {
	process_env_path("path");
	process_env_path("include");
	process_env_path("lib");
	process_env_path("libpath");

	write_vcvarsportable();
}

int main(int argc, char **argv) {
	generate();

	return 0;
}