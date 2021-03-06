
//PA6H GPRMC
//BN220 GNRMC

//#define BN220
#define PA6H

#ifdef PA6H
#undef BN220
#else
#define BN220
#endif

void PUT32 ( unsigned int, unsigned int );
unsigned int GET32 ( unsigned int );
void dummy ( unsigned int );

#define GPIOABASE 0x48000000
#define GPIOA_MODER (GPIOABASE+0x00)

#define RCCBASE     0x40021000

#define USART2BASE  0x40004400

#define RCC_CR      (RCCBASE+0x00)

static unsigned char num[8];

static int uart_init ( void )
{
    unsigned int ra;

    ra=GET32(RCCBASE+0x14);
    ra|=1<<17; //enable port a
    PUT32(RCCBASE+0x14,ra);

    ra=GET32(RCCBASE+0x1C);
    ra|=1<<17; //enable USART2
    PUT32(RCCBASE+0x1C,ra);

    //PA2 UART1_TX
    //PA3 UART1_RX

    //moder
    ra=GET32(GPIOABASE+0x00);
    ra&=~(3<<(2<<1)); //PA2
    ra|=  2<<(2<<1);  //PA2
    ra&=~(3<<(3<<1)); //PA3
    ra|=  2<<(3<<1);  //PA3
    PUT32(GPIOABASE+0x00,ra);

    //afr
    ra=GET32(GPIOABASE+0x20);
    ra&=~(0xF<< 8); //PA2
    ra|=  0x1<< 8;  //PA2
    ra&=~(0xF<<12); //PA3
    ra|=  0x1<<12;  //PA3
    PUT32(GPIOABASE+0x20,ra);

    ra=GET32(RCCBASE+0x10);
    ra|=1<<17; //reset USART2
    PUT32(RCCBASE+0x10,ra);
    ra&=~(1<<17);
    PUT32(RCCBASE+0x10,ra);

    //PUT32(USART2BASE+0x0C,1667); //4800
    PUT32(USART2BASE+0x0C,833); //9600
    PUT32(USART2BASE+0x08,1<<12);
    PUT32(USART2BASE+0x00,(1<<3)|(1<<2)|1);

    return(0);
}

static unsigned int uart_recv ( void )
{
    while(1) if((GET32(USART2BASE+0x1C))&(1<<5)) break;
    return(GET32(USART2BASE+0x24));
}

static void uart_send ( unsigned int x )
{
    while(1) if(GET32(USART2BASE+0x1C)&(1<<7)) break;
    PUT32(USART2BASE+0x28,x);
}

static void show_number ( void )
{
    //if(num[0]==0) num[0]=10;
    uart_send('X');
    uart_send(num[0]+0x30);
    uart_send(num[1]+0x30);
    uart_send(num[2]+0x30);
    uart_send(num[3]+0x30);
    uart_send(num[4]+0x30);
    uart_send(num[5]+0x30);
    uart_send('Y');
    uart_send(0x0D);
    uart_send(0x0A);
}


//$GPRMC,000143.00,A,4030.97866,N,07955.13947,W,0.419,,020416,,,A*6E
static unsigned int timezone;
static int do_nmea ( void )
{
    unsigned int ra;
    unsigned int rb;
    unsigned int rc;
    unsigned int state;
    unsigned int off;
    unsigned int sum;
    unsigned int savesum;
    unsigned int xsum;
    unsigned int validity;
    //unsigned int num[4];
    unsigned char xstring[32];


    state=0;
    off=0;
//$GPRMC,054033.00,V,
    sum=0;
    savesum=0;
    xsum=0;
    validity=0;
    while(1)
    {
        ra=uart_recv();
        //uart_send(ra);
        //uart_send(0x30+state);
        //PUT32(USART2BASE+0x28,ra);

        if(ra!='*') sum^=ra;

        switch(state)
        {
            case 0:
            {
                if(ra=='$') state++;
                else state=0;
                sum=0;
                break;
            }
            case 1:
            {
                if(ra=='G') state++;
                else state=0;
                break;
            }
            case 2:
            {
//should I just ignore this character and keep going?
#ifdef PA6H
                if(ra=='P') state++;
#endif
#ifdef BN220
                if(ra=='N') state++;
#endif
                else state=0;
                break;
            }
            case 3:
            {
                if(ra=='R') state++;
                else state=0;
                break;
            }
            case 4:
            {
                if(ra=='M') state++;
                else state=0;
                break;
            }
            case 5:
            {
                if(ra=='C') state++;
                else state=0;
                break;
            }
            case 6:
            {
                off=0;
                if(ra==',') state++;
                else state=0;
                break;
            }
            case 7:
            {
                if(ra==',')
                {
                    if(off>7)
                    {
                        //xstring[0] //tens of hours
                        //xstring[1] //ones of hours
                        //xstring[2] //tens of minutes
                        //xstring[3] //ones of minutes
                        //xstring[4] //tens of seconds
                        //xstring[5] //ones of seconds

                        rb=xstring[0]&0xF;
                        rc=xstring[1]&0xF;
                        //1010
                        rb=/*rb*10*/(rb<<3)+(rb<<1); //times 10
                        rb+=rc;
                        if(rb>12) rb-=12;
                        //ra=5; //time zone adjustment winter
                        //ra=4; //time zone adjustment summer
                        ra=timezone;
                        if(rb<=ra) rb+=12;
                        rb-=ra;
                        if(rb>9)
                        {
                            xstring[0]='1';
                            rb-=10;
                        }
                        else
                        {
                            xstring[0]='0';
                        }
                        rb&=0xF;
                        xstring[1]=0x30+rb;
                        rb=0;
                        //hour  =((xstring[0]&0xF)*10)+(xstring[1]&0xF);
                        //minute=((xstring[2]&0xF)*10)+(xstring[3]&0xF);
                        //second=((xstring[4]&0xF)*10)+(xstring[5]&0xF);
                        //hexstrings(hour);
                        //hexstrings(minute);
                        //hexstring(second);
                    }
                    off=0;
                    state++;
                }
                else
                {
                    if(off<16)
                    {
                        xstring[off++]=ra;
                    }
                }
                break;
            }
            case 8:
            {
                if(ra=='A') validity=1;
                else validity=0;
                state++;
            }
            case 9:
            {
                if(ra=='*')
                {
                    savesum=sum;
                    state++;
                }
                break;
            }
            case 10:
            {
                if(ra>0x39) ra-=0x37;
                xsum=(ra&0xF)<<4;
                state++;
                break;
            }
            case 11:
            {
                if(ra>0x39) ra-=0x37;
                xsum|=(ra&0xF);
                if(savesum==xsum)
                {
                    num[0]=xstring[0]&0xF;
                    num[1]=xstring[1]&0xF;
                    num[2]=xstring[2]&0xF;
                    num[3]=xstring[3]&0xF;
                    num[4]=xstring[4]&0xF;
                    num[5]=xstring[5]&0xF;

                    //just have the colon show validity state
                    if(validity)
                    {
                    }
                    else
                    {
                        //num[3]|=0x40;
                    }
                    show_number();

                }
                for(rb=0;rb<6;rb++) xstring[rb]=0;
                state=0;
                break;
            }
        }
    }
    return(0);
}

int notmain ( void )
{
    unsigned int ra;
    unsigned int rb;

    //timezone=4;
    //rb=0x44444444;
    //ra=GET32(0x20000D00);
    //if(ra==0x44444444)
    //{
        //timezone=5; //EST
        //rb=0x55555555;
    //}
    //else
    //if(ra==0x55555555)
    //{
        //timezone=4; //EDT
        //rb=0x44444444;
    //}

    timezone=0;
    rb=0;
    ra=GET32(0x20000D00);
    switch(ra)
    {
        case 0x44444444:
        {
            timezone=5; //EST
            rb=0x55555555;
            break;
        }
        case 0x55555555:
        {
            timezone=7; //Arizona
            rb=0x77777777;
            break;
        }
        case 0x77777777:
        {
            timezone=4; //EDT
            rb=0x44444444;
            break;
        }
        default:
        {
            timezone=7; //Arizona
            rb=0x77777777;
            break;
        }
    }
    PUT32(0x20000D00,rb);

    num[0]=1;
    num[1]=2;
    num[2]=3;
    num[3]=4;
    num[4]=5;
    num[5]=6;

    uart_init();
    show_number();
    do_nmea();

    return(0);
}

/*

dfu-util -a 0 -s 0x08000000 -D notmain.bin

*/


