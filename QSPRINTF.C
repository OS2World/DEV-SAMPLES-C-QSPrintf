/*
** qsprintf()
** Copyright (C) 1988, 1989, William B. McCormick
** Released to public domain on 01 February 1989.
**
** Reentrant sprintf() replacement.  Quick-and-dirty but duplicates most
** of the sprintf() functionality.
**
** History:
**      07/28/88        William B. McCormick
**              Quick printf() for OS/2 multi-threaded programs.
**      02/01/89        William B. McCormick
**              Changed to sprintf().  Added '*' specification.  Released
**              to public domain.
**
** Properly formats:
**      % [flags] [width ] [.precision] [size] type
**          flags:
**              -       left justify the result within field.
**              0       pad with '0' instead of ' '
**          width:
**              Controls the minimum # of characters printed.  If value is
**              less than specified width, padding is added.  The width
**              may be '*', in which case an integer argument from the
**              argument list supplies the value.
**          precision:
**              Specifies the maximum # of characters printed.  The precision
**              may be '*', in which case an integer argument from the
**              argument list supplies the value.
**          size:
**              F       Specifies 's' is FAR
**              N       Specifies 's' is NEAR
**              l       Specifies 'd', 'x', 'u' is long
**              B       Followed by integer (or '*') from 2-36.  Specifies
**                      base of number to be printed with 'd', 'i', or 'u'.
**          type:
**              s       string
**              d       signed integer
**              i       signed integer
**              u       unsigned integer
**              x       unsigned hexadecimal integer ('abcdef')
**              n       Number of characters written to destination so far
**                      is stored in the integer whose address is given as
**                      the argument
*/

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>


        /* comment out these lines for strict ANSI compatibility */
#define FAR     far
#define NEAR    near


        /* prototype */
int qsprintf( char *dest, const char *fmt, ... );


#define TRUE 1
#define FALSE 0

#ifndef isdigit
#       define isdigit(c) ((c)>='0' && (c)<='9')
#endif


#define w_char( dest, ch )      (*(dest)++ = ch)
#define w_str( dest, str, len ) for ( ; (len)>0; (len)--, (str)++ ) \
                                        w_char( (dest), *(str) )
        

static void
w_string( char **pdest,
          register const char FAR *str,
          int minlen,
          int maxlen,
          char fill )
{
unsigned count, c2;
register const char FAR *p;
char    *dest;
        dest = *pdest;

        count = 0;
        p = str;
        while ( *p++ )
                if ( ++count == maxlen )
                        break;

        if ( minlen > 0 ) {
                for ( ; minlen>count; --minlen )
                        w_char( dest, fill );
        }

        c2 = count;
        w_str( dest, str, c2 );

        if ( minlen < 0 ) {
                for ( minlen=-minlen; minlen>count; --minlen )
                        w_char( dest, fill );
        }
        *pdest = dest;
}


static char
get_int( va_list *parg, const char **pp, int *vp )
{
register const char *p;
register int val;
int sign;
char firstc;
        p = *pp;

        if ( *p=='-' ) {
                ++p;
                sign = -1;
        }
        else
                sign = 1;
        firstc = *p;

        if ( *p=='0' )          /* skip zero in case '0*' */
                ++ p;

        if ( *p=='*' ) {        /* get the value */
                ++ p;
                val = va_arg( *parg, int );
        }
        else {
                val = 0;
                for ( ; isdigit(*p); p++ )
                        val = val * 10 + *p - '0';
        }
        *vp = val * sign;
        *pp = p;
        return firstc;
}




int cdecl
qsprintf( char *dest, const char *fmt, ... )
{
va_list                 arg;
register const char     *reg_p;
const char              *auto_p;
char                    buf[ CHAR_BIT * sizeof(long) + 1 ];
int                     val, val2;
int                     base;
int                     islong, isfar, isnear;
char                    fill;
char                    *org_dest;

    org_dest = dest;

    va_start(arg,fmt);
    reg_p = fmt;

    for ( ;*reg_p; reg_p++ ) {
        if ( *reg_p=='%' ) {
            isnear = isfar = islong = FALSE;
            val = val2 = 0;
            base = 10;
            fill = ' ';
            auto_p = reg_p + 1;
            if ( get_int( &arg, &auto_p, &val ) == '0' )
                fill = '0';
            if ( *auto_p=='.' ) {
                ++auto_p;
                (void) get_int( &arg, &auto_p, &val2 );
            }
            reg_p = auto_p-1;
next_fmt:
            switch ( *++reg_p ) {

                case 'l':
                    islong = TRUE;
                    goto next_fmt;

                case 'F':
                    isfar = TRUE;
                    goto next_fmt;

                case 'N':
                    isnear = TRUE;
                    goto next_fmt;

                case 'B':
                    auto_p = reg_p+1;
                    (void) get_int( &arg, &auto_p, &base );
                    reg_p = auto_p-1;
                    goto next_fmt;

                case 'n':
                    *va_arg( arg, int * ) = dest - org_dest;
                    break;

                case 's':
                    w_string( &dest,
                              isfar ?
                                va_arg(arg,const char FAR *) :
                                (isnear ?
                                 (const char FAR *)va_arg(arg,
                                                          const char NEAR *) :
                                 (const char FAR *)va_arg(arg, const char *)),
                              val,
                              val2,
                              fill );
                    break;
                                    
                case 'u':
                    w_string( &dest,
                              ltoa( islong ?
                                     va_arg(arg,unsigned long) :
                                     (unsigned long)va_arg(arg,unsigned),
                                    buf,
                                    base ),
                              val,
                              INT_MAX,
                              fill );
                    break;

                case 'd':
                case 'i':
                    w_string( &dest,
                              ltoa( islong ?
                                     va_arg(arg,long) :
                                     (long) va_arg(arg,int),
                                    buf,
                                    base ),
                              val,
                              INT_MAX,
                              fill );
                    break;

                case 'x':
                    w_string( &dest,
                              ltoa( islong ?
                                     va_arg(arg,long) :
                                     (long) va_arg(arg,int),
                                    buf,
                                    16 ),
                              val,
                              INT_MAX,
                              fill );
                    break;
                                    
                default:
                    w_char( dest, *reg_p );
            }
        }
        else {
            w_char( dest, *reg_p );
        }
    }
    w_char( dest, '\0' );
    return dest - org_dest - 1;         /* length of string */
}





