static char* 
parseNonWsp(char* p) {
	char* result = p;
	while(*result != 0 && (*result >= '!' && *result <= '}')) {
		result++;
	}
	return(result);
}

static char* 
parseWsp(char *p) {
	char* result = p;
	while(*result != 0 && (*result == ' ' || *result == '\n' || *result == '\t'
		|| *result == '\v' || *result == '\f' || *result == '\r')) {
		result++;
	}
	return(result);
}

static bool 
checkExtensionString(char* extensionString, char* check) {
	char* pointer = extensionString;
	char* end;
	puts("START PARSING");
	while(*pointer != 0) {
		pointer = parseWsp(pointer);
		end = parseNonWsp(pointer);
		if((unsigned int)(end - pointer) == strlen(check)) {
			char* c = check;
			for(; pointer <= end; pointer++, c++) {
				if(*pointer != *c) { 
					break;
				} else {
					return(true);
				}
			}
		}
		pointer = end;
	}
	return(false);
}