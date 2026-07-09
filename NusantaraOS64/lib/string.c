/*
 * NusantaraOS64 - String Library
 * 
 * Basic string manipulation functions
 */

#include "../include/nusantara.h"

/*
 * Calculate string length
 */
size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

/*
 * Copy string
 */
char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

/*
 * Compare strings
 */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/*
 * Convert integer to ASCII string
 */
char *itoa(int value, char *str, int base) {
    char *p = str;
    char *p1, *p2;
    unsigned long uvalue;
    char tmp;
    
    /* Handle negative numbers for base 10 */
    if (base == 10 && value < 0) {
        *p++ = '-';
        uvalue = -value;
    } else {
        uvalue = value;
    }
    
    p1 = p;
    
    /* Convert number */
    do {
        int digit = uvalue % base;
        *p++ = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        uvalue /= base;
    } while (uvalue);
    
    *p = '\0';
    
    /* Reverse the string */
    p2 = p - 1;
    while (p1 < p2) {
        tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
    }
    
    return str;
}

/*
 * Convert long to ASCII string
 */
char *ltoa(long value, char *str, int base) {
    char *p = str;
    char *p1, *p2;
    unsigned long uvalue;
    char tmp;
    
    if (base == 10 && value < 0) {
        *p++ = '-';
        uvalue = -value;
    } else {
        uvalue = value;
    }
    
    p1 = p;
    
    do {
        int digit = uvalue % base;
        *p++ = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        uvalue /= base;
    } while (uvalue);
    
    *p = '\0';
    
    p2 = p - 1;
    while (p1 < p2) {
        tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
    }
    
    return str;
}

/*
 * Convert hex string to integer
 */
unsigned long hextoul(const char *str) {
    unsigned long result = 0;
    char c;
    
    /* Skip 0x prefix if present */
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }
    
    while ((c = *str++) != '\0') {
        result *= 16;
        if (c >= '0' && c <= '9') {
            result += c - '0';
        } else if (c >= 'a' && c <= 'f') {
            result += c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            result += c - 'A' + 10;
        }
    }
    
    return result;
}

/*
 * Convert decimal string to integer
 */
int atoi(const char *str) {
    int result = 0;
    int sign = 1;
    
    /* Handle negative sign */
    if (*str == '-') {
        sign = -1;
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}
