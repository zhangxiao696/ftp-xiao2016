#include "string_operate.h"

void str_trim_crlf(char *str)
{
	char *p = &str[strlen(str)-1];
	while(*p=='\r' || *p=='\n')
		*p-- = '\0';
}

void str_split(const char *str, char *left, char *right, char c)
{
	char *p = strchr(str, c);
	if(p == NULL)
		strcpy(left, str);
	else
	{
		strncpy(left, str, p-str);
		strcpy(right, p+1);
	}
}

int str_all_space(const char *str)
{
	while(*str)
	{
		if(!isspace(*str))
			return 0;
		str++;
	}
	return 1;
}

void str_upper(char *str)
{
	while(*str)
	{
		*str = toupper(*str);
		str++;
	}
}

long long str_to_longlong(const char *str)
{
	//return atoll(str);   //this is a easy method

	//this method is came ture by myself
	long long sum = 0;
	long long mult = 1;
	int len = strlen(str);
	if(len > 15)
		return 0;
    int i;
	for(i = len-1; i>=0; i--)
	{
		if(str[i]<'0' || str[i]>'9')
			return 0;

/*		sum += (str[i]*mult);
		mult *= 10;*/    //this way is wrong

		char ch = str[i];
		long long val = ch - '0';
		val *= mult;
		sum += val;
		mult *= 10;
	}

	return sum;
}

unsigned int str_octal_to_uint(const char *str)
{
	/*this is the first method*/
// 	unsigned int sum = 0;
// 	unsigned int mult = 1;
// 	int len = strlen(str);
// /*	if(len > 15)
// 		return 0;*/

//     int i;
// 	for(i = len-1; i>=0; i--)
// 	{
// 		if(str[i]<'0' || str[i]>'9')
// 			return 0;
// 		char ch = str[i];
// 		long long val = ch - '0';
// 		val *= mult;
// 		sum += val;
// 		mult *= 8;
// 	}

	/*this is the second method*/
	unsigned int result = 0;
	while(str != NULL)
	{
		int digit = *str;
		if(!(isdigit(digit)) || digit > '7')
			break;

		if(digit != '0')
		{
			result = result << 3;
			//result += digit;    this style is wrong!!!!!!!!!
			result += (digit - '0');
		}

		str++;
	}

	return result;
}