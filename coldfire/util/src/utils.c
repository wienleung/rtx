#include "rtx.h"
#include "utils.h"

int _findChar (CHAR ch, const CHAR * str);

CHAR * rtx_sprintf(CHAR * str, const CHAR * format, void * params[])
{
	int i = 0, j = 0, k = 0;
	while (format[k] != '\0')
	{
		if(format[k] == '%')
		{
            k++;
            int spaces = format[k] - '0';
            if(spaces < 10 && spaces > 0)
                k++;
            else
                spaces = 0;
			if(params[i] != NULL)
			{
				if(format[k] == 'c')
				{
					//if((params[i])[0] == NULL)
					//	return ERROR;
                    
                    while(spaces > 1)
                    {
                        str[j] = ' ';
                        j++;
                        spaces--;
                    }  
                    
					str[j] = (params[i])[0];
					i++;
					j++;
					k++;                      
				}
				else if(format[k] == 's')
				{
					CHAR * p_str = params[i];
					i++;
					int temp_i = 0;
					while (p_str[temp_i] != '\0')
					{
						str[j] = p_str[temp_i];
						j++;
						temp_i++;
					}
					k++;
				}
				else if(format[k] == 'i')
				{
					int size = 0, div = 1;
					int num = *((int *) params[i]);
					i++;
					while(num/div > 0)
					{
						size++;
						div*=10;
					}
					div/=10;

					if(size == 0)
					{
						str[j] = '0';
						j++;
					}
                    
                    if(size < spaces)
                    {	
                        while(spaces > size)
                        {
                            str[j] = ' ';
                            j++;
                            spaces--;
                        }
                    }
                    
                    while(size > 0)
                    {
                        int digit = num/div;
                        str[j] = '0' + digit;
                        j++;
                        size--;
                        num -= digit*div;
                        div/=10;
                    }
					k++;
				}
				else
				{
					str[j] = '%';
					j++;
					str[j] = format[k];
					j++;
					k++;
				}
			}
			else
			{
				return NULL;
			}		
		}
		else
		{
			str[j] = format[k];
			k++;
			j++;
		}
		str[j] = '\0';
	}
	return str;
}

CHAR * rtx_strcpy(CHAR * str, const CHAR * cpy_str)
{
	void * params[] = {cpy_str};
	return rtx_sprintf(CHAR * str, "%s", params);
}

int rtx_strcmp(const CHAR * str1, const CHAR * str2)
{
	if(str1 == NULL || str2 == NULL)
		return -1;
	
	int k = 0;
	while (str1[k] != '\0' && str2[k] != '\0')
	{
		if(str1[k] != str2[k])
			return 1;
        k++;
	}
	return 0;
}

CHAR * rtx_strtok(CHAR * str, const CHAR * delimiters)
{
    static char copy [100] = "";
    static BYTE i = 0;
    static CHAR ret [10] = "";

    if ( str != NULL )
    {
        rtx_strcpy ( copy, str );
        i = 0;
    }

    if ( *copy == '\0' )
    {
        return NULL;
    }

    BYTE j = 0;
    
    //skip leading characters to be ignored
    while ( _findChar( copy[i], delimiters ) == 1 )
    {
        i++;
    }
    
    while ( copy[i] != '\0' && _findChar( copy[i], delimiters ) == 0 )
    {
        ret[j] = copy [i];
        i++;
        j++;
    }
    ret[j]='\0';
    
    return ret; 
}

int _findChar (CHAR ch, const CHAR * str)
{
    if (str == NULL)
    {
        return -1;
    }

    int i = 0;
    while ( str[i] != '\0' )
    {
        if ( ch == str[i] )
        {
            return 1;
        }
        i++;
    }
    return 0;
}