/*** Zufallszahlen-Tabelle mit 1024 Zahlen nach stdout ***/
/*** die Zahlen liegen im Bereich -1..+1               ***/
#include <exec/types.h>
#include <stdio.h>
#include <math.h>

int main(void)
{
    ULONG i,j;
    ULONG pos,neg;

    pos = 0;
    neg = 0;
    for (i=0; i<256; i++) {
        for (j=0; j<4; j++) {

            double r;
            r = drand48();
            r = 2*r - 1.0;

            if (r<0) neg++;
            else     pos++;

            printf("%f, ",r);
        };
        printf("\n");
    };

    printf("\n\n num_pos = %d, num_neg = %d\n",pos,neg);

    return 0;
}





