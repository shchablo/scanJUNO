#include "simple_server.h"
#include "math.h"
#define PSTR(s) s

static unsigned char mymac[6] = { 0x54, 0x55, 0x58, 0x10, 0x00, 0x24 }; // mac
static unsigned char myip[4] = { 192, 168, 1, 4 }; // ip
static char baseurl[] = "http://192.168.1.4/"; // a DNS server (baseurl must end in "/"):
static unsigned int mywwwport = 80; // listen port for tcp/www (max range 1-254)
static unsigned int myudpport = 1200; // listen port for udp

#define BUFFER_SIZE 1500
static unsigned char buf[BUFFER_SIZE + 1];

// the password string (only the first 5 char checked), (only a-z,0-9,_ characters):
static char password[] = "888"; // must not be longer than 9 char


static unsigned char Enc28j60Bank;
static alt_u16 NextPacketPtr;

void SPI2_Write(unsigned char writedat)
{
	alt_avalon_spi_command(LAN_BASE,0,1,&writedat,0,NULL,ALT_AVALON_SPI_COMMAND_MERGE);
}
unsigned char SPI2_Read()
{
	alt_u8 temp;
	alt_avalon_spi_command(LAN_BASE,0,0,NULL,1,&temp,ALT_AVALON_SPI_COMMAND_MERGE);

	return temp;
}


unsigned char enc28j60ReadOp(unsigned char op, unsigned char address)
{
	unsigned char dat = 0;

	ENC28J60_CSL();

	dat = op | (address & ADDR_MASK);
	SPInet_Write(dat);
	dat = SPInet_Read();
   // do dummy read if needed (for mac and mii, see datasheet page 29)
	if(address & 0x80) {
      dat = SPInet_Read(0xFF);
	}
    // release CS
	ENC28J60_CSH();

	return dat;
}

void enc28j60WriteOp(unsigned char op, unsigned char address, unsigned char data)
{
	unsigned char dat = 0;

	ENC28J60_CSL();
	// issue write command
	dat = op | (address & ADDR_MASK);
	SPInet_Write(dat);
	// write data
	dat = data;
	SPInet_Write(dat);
	ENC28J60_CSH();
}

void enc28j60ReadBuffer(alt_u16 len, unsigned char* data)
{
	ENC28J60_CSL();
	// issue read command
	SPInet_Write(ENC28J60_READ_BUF_MEM);
	while (len--) {
		*data++ = (unsigned char) SPInet_Read( );
	}
	*data = '\0';
	ENC28J60_CSH();
}

void enc28j60WriteBuffer(alt_u16 len, unsigned char* data)
{
	ENC28J60_CSL();
	// issue write command
	SPInet_Write(ENC28J60_WRITE_BUF_MEM);

	while (len--) {
		SPInet_Write(*data++);
	}
	ENC28J60_CSH();
}

void enc28j60SetBank(unsigned char address)
{
	// set the bank (if needed)
	if((address & BANK_MASK) != Enc28j60Bank) {
		// set the bank
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1 | ECON1_BSEL0));
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK) >> 5);
		Enc28j60Bank = (address & BANK_MASK);
	}
}

unsigned char enc28j60Read(unsigned char address)
{
	// set the bank
	enc28j60SetBank(address);
	// do the read
	return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

void enc28j60Write(unsigned char address, unsigned char data)
{
	// set the bank
	enc28j60SetBank(address);
	// do the write
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

void enc28j60PhyWrite(unsigned char address, alt_u16 data)
{
	// set the PHY register address
	enc28j60Write(MIREGADR, address);
	// write the PHY data
	enc28j60Write(MIWRL, data);
	enc28j60Write(MIWRH, data >> 8);
	// wait until the PHY write completes
	while(enc28j60Read(MISTAT) & MISTAT_BUSY) {
    }
}

void enc28j60clkout(unsigned char clk)
{
	// setup clkout: 2 is 12.5MHz:
	enc28j60Write(ECOCON, clk & 0x7);
}

void enc28j60Init(unsigned char * macaddr)
{
	unsigned long i;

	ENC28J60_RSTH();
	for(i = 0; i < 1000; i++);
	ENC28J60_RSTL();
	for(i = 0; i < 10000; i++);
	ENC28J60_RSTH();
	for (i = 0; i < 10000; i++);


	// initialize I/O
	ENC28J60_CSH();
	// perform system reset
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);

	NextPacketPtr = RXSTART_INIT;
	// Rx start
	enc28j60Write(ERXSTL, RXSTART_INIT & 0xFF);
	enc28j60Write(ERXSTH, RXSTART_INIT >> 8);
	// set receive pointer address
	enc28j60Write(ERXRDPTL, RXSTART_INIT & 0xFF);
	enc28j60Write(ERXRDPTH, RXSTART_INIT >> 8);
	// RX end
	enc28j60Write(ERXNDL, RXSTOP_INIT & 0xFF);
	enc28j60Write(ERXNDH, RXSTOP_INIT >> 8);
	// TX start
	enc28j60Write(ETXSTL, TXSTART_INIT & 0xFF);
	enc28j60Write(ETXSTH, TXSTART_INIT >> 8);
	// TX end
	enc28j60Write(ETXNDL, TXSTOP_INIT & 0xFF);
	enc28j60Write(ETXNDH, TXSTOP_INIT >> 8);

	enc28j60Write(ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_PMEN);
	enc28j60Write(EPMM0, 0x3f);
	enc28j60Write(EPMM1, 0x30);
	enc28j60Write(EPMCSL, 0xf9);
	enc28j60Write(EPMCSH, 0xf7);
   //
   // do bank 2 stuff
   // enable MAC receive
   enc28j60Write(MACON1, MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);
   // bring MAC out of reset
   enc28j60Write(MACON2, 0x00);
   // enable automatic padding to 60bytes and CRC operations
   enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN | MACON3_FULDPX);
   // set inter-frame gap (non-back-to-back)
   enc28j60Write(MAIPGL, 0x12);
   enc28j60Write(MAIPGH, 0x0C);
   // set inter-frame gap (back-to-back)
   enc28j60Write(MABBIPG, 0x12);
   // Set the maximum packet size which the controller will accept
   // Do not send packets longer than MAX_FRAMELEN:
   enc28j60Write(MAMXFLL, MAX_FRAMELEN & 0xFF); 
   enc28j60Write(MAMXFLH, MAX_FRAMELEN >> 8);
   // do bank 3 stuff
   // write MAC address
   // NOTE: MAC address in ENC28J60 is byte-backward
   enc28j60Write(MAADR5, macaddr[0]);  
   enc28j60Write(MAADR4, macaddr[1]);
   enc28j60Write(MAADR3, macaddr[2]);
   enc28j60Write(MAADR2, macaddr[3]);
   enc28j60Write(MAADR1, macaddr[4]);
   enc28j60Write(MAADR0, macaddr[5]);

   enc28j60PhyWrite(PHCON1, PHCON1_PDPXMD);


   // no loopback of transmitted frames
   enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);
   // switch to bank 0
   enc28j60SetBank(ECON1);
   // enable interrutps
   enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE | EIE_PKTIE);
   // enable packet reception
   enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}

// read the revision of the chip:
unsigned char enc28j60getrev(void)
{
	return(enc28j60Read(EREVID));
}

void enc28j60PacketSend(unsigned int len, unsigned char* packet)
{
	// Set the write pointer to start of transmit buffer area
	enc28j60Write(EWRPTL, TXSTART_INIT & 0xFF);
	enc28j60Write(EWRPTH, TXSTART_INIT >> 8);

	// Set the TXND pointer to correspond to the packet size given
	enc28j60Write(ETXNDL, (TXSTART_INIT + len) & 0xFF);
	enc28j60Write(ETXNDH, (TXSTART_INIT + len) >> 8);

	// write per-packet control byte (0x00 means use macon3 settings)
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);

	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

	// Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
	if((enc28j60Read(EIR) & EIR_TXERIF)) {
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
	}
}

/*-----------------------------------------------------------------
 Gets a packet from the network receive buffer, if one is available.
 The packet will by headed by an ethernet header.
      maxlen  The maximum acceptable length of a retrieved packet.
      packet  Pointer where packet data should be stored.
 Returns: Packet length in bytes if a packet was retrieved, zero otherwise.
-------------------------------------------------------------------*/
alt_u16 enc28j60PacketReceive(alt_u16 maxlen, unsigned char* packet)
{
	unsigned int rxstat;
	unsigned int len;

	if(enc28j60Read(EPKTCNT) == 0) {
		return(0);
	}

	// Set the read pointer to the start of the received packet
	enc28j60Write(ERDPTL, (NextPacketPtr));
	enc28j60Write(ERDPTH, (NextPacketPtr) >> 8);

	// read the next packet pointer
	NextPacketPtr = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0) << 8;

	// read the packet length (see datasheet page 43)
	len = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0) << 8;

	len -= 4; //remove the CRC count
	// read the receive status (see datasheet page 43)
	rxstat = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0) << 8;
	// limit retrieve length
	if(len > maxlen - 1) {
		len = maxlen - 1;
	}

	// check CRC and symbol errors (see datasheet page 44, table 7-3):
	// The ERXFCON.CRCEN is set by default. Normally we should not
	// need to check this.
	if((rxstat & 0x80) == 0) {
		// invalid
		len = 0;
	}
	else {
		// copy the packet from the receive buffer
		enc28j60ReadBuffer(len, packet);
	}
	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
   enc28j60Write(ERXRDPTL, (NextPacketPtr));
   enc28j60Write(ERXRDPTH, (NextPacketPtr) >> 8);

   // decrement the packet counter indicate we are done with this packet
   enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
   return(len);
}


unsigned char verify_password(char* str)
{
	// the first characters of the received string are
	// a simple password/cookie:
   if(strncmp(password, str, sizeof(password)-1) == 0) {
	   return(1);
   }
   return(0);
}

// takes a string of the form password/commandNumber and analyse it
// return values: -1 invalid password, otherwise command number
//                -2 no command given but password valid
unsigned char analyse_get_url(char* str)
{
	unsigned char i = 0;
	if(verify_password(str) == 0) {
		return((unsigned char)-1);
	}
   // find first "/"
   // passw not longer than 9 char:
   while (*str && i<10 && *str>',' && *str < '{') {
      if(*str == '/') {
    	  str++;
    	  break;
      }
      i++;
      str++;
   }
   if(*str <0x3a && *str> 0x2f) {
      // is a ASCII number, return it
	   return(*str - 0x30);
   }

   return((unsigned char)-2);
}

void main_page(unsigned int *len)
{
	*len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
	*len = fill_tcp_data_p(buf, *len, PSTR(""
	"<html>"
	"<head>"
	"<link rel='stylesheet' href='http://192.168.1.4/basic.css' type='text/css' />"
	"<title>webscan</title>"
	"<script type='text/javascript'></script>"
	"<script type='text/javascript' src='http://code.jquery.com/jquery-2.2.4.js'> </script>"
	"<script type='text/javascript' src='http://code.highcharts.com/highcharts.js'> </script>"
	"<script type='text/javascript' src='http://192.168.1.4/counter.js'> </script>"
	"</head>"
	"<body>"
		"<div id='wrapper'>"
			"<div id='header'>"
            	"<p style='text-align: center;'>WEBSCAN</p>"
            "</div>"
			"<div id='center_wrap'>"
				"<div id='chart'></div>"
				"<div id='set_stat'>"
					"<div id='status'>"
		//				"<div id='s1'>""</div>"
		//				"<div id='s2'>""</div>"
						"<div id='s3'>""</div>"
						"<div id='s4'>""</div>"
						"<div id='s5'>""</div>"
						"<div id='s6'>""</div>"
						"<div id='foo'>""</div>"
					"</div>"
					"<div id='set1'></div>"
					"<div id='set2'></div>"
				"</div>"
			"</div>"
		"</div>"
		"<script>"
		"$( '#set1' ).load( 'http://192.168.1.4/set1.html' );"
		"$( '#set2' ).load( 'http://192.168.1.4/set2.html' );"
		"$( '#foo' ).load( 'http://192.168.1.4/pages.html' );"
		"</script>"
	"<script type='text/javascript' src='http://192.168.1.4/but1.js'> </script>"
	"<script type='text/javascript' src='http://192.168.1.4/but2.js'> </script>"
	"</body>"
	"</html>"));
}

int simple_server()
{

	// common
    unsigned char valuelong[32] = {0};
    unsigned char value[3] = {0};
    int valueInt = 0;
	char array[32] = {0};
	int delay = 50000; // ml sec.

	// dac
	unsigned int dacTime = 0; // sec
	int dacInt= 0; // sec

	// addr
	unsigned char addrChar = 0; IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, addrChar);
	int addrInt = 0;
	char addrArray[3] = {0};
	unsigned char addrTmp = 0;


	// data
	unsigned char wdata = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, wdata);
	int wdataInt = 0;

	unsigned char rdata = 0x00; rdata = IORD_ALTERA_AVALON_PIO_DATA(PIO_RDATA_BASE);
	char signals = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals);

	unsigned int count; count = IORD_ALTERA_AVALON_PIO_DATA(PIO_COUNT_BASE);
	unsigned int time; time = IORD_ALTERA_AVALON_PIO_DATA(PIO_TIME_BASE);


	float freq;
	unsigned char gen_zero[4];
	unsigned char gen_sig[4];
	int showFreq = 0;

	unsigned int i,j,k;
	unsigned int plen;
	unsigned int dat_p;
	unsigned int cmd;
    unsigned char cmd_pos = 0;
    unsigned char payloadlen = 0;
    char str[30];
    char cmdval;

    float live_time_count;



    int iBuf = 0;
    int iiBuf = 0;
    int dacBuf[200] = {0};

   /*initialize enc28j60*/
   enc28j60Init(mymac);
   
   str[0]=(char)enc28j60getrev();
   
   init_ip_arp_udp_tcp(mymac, myip, mywwwport);
   enc28j60PhyWrite(PHLCON, 0x476);
   enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz

   // init the ethernet/ip layer:
   while (1) {

	   plen = enc28j60PacketReceive(BUFFER_SIZE, buf);
	   if(plen == 0) {
		   continue;
	   }
		
	   if(eth_type_is_arp_and_my_ip(buf, plen)) {
    	  make_arp_answer_from_request(buf);
    	  continue;
	   }

	   if(eth_type_is_ip_and_my_ip(buf, plen) == 0) {
    	  continue;
       }

	   if(buf[IP_PROTO_P] == IP_PROTO_ICMP_V &&
          buf[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V) {
		   make_echo_reply_from_request(buf, plen);
		   continue;
       }

	   if(buf[IP_PROTO_P] == IP_PROTO_TCP_V &&
		  buf[TCP_DST_PORT_H_P] == 0 &&
          buf[TCP_DST_PORT_L_P] == mywwwport) {
		   if(buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V) {
			   make_tcp_synack_from_syn(buf);
			   continue;
		   }
		   if(buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V) {
            init_len_info(buf);
            dat_p = get_tcp_data_pointer();
            if(dat_p == 0) {
            	if(buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V) {
            	   make_tcp_ack_from_any(buf);
                }
               continue;
            }

            // get requst
            if(strncmp("GET ", (char *) &(buf[dat_p]), 4) != 0) {

            	// post requst
            	if(strncmp("POST ", (char *) &(buf[dat_p]), 5) == 0) {
            		for(i = dat_p; i < 1400; i++) {
            			if((strncmp("login=", (char *) &(buf[i]), 6) == 0)) {
            				if(strncmp("pmtlab&pass=amsams", (char *) &(buf[i+6]), 18) == 0) { // parsing login and password
            					main_page(&plen);
            					goto SENDTCP;
            				}
            				else { // страница не авторизации
            					plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
            					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>Password isn't correct!</p>"));
            					plen = fill_tcp_data_p(buf, plen, PSTR(""
            							"<p style='text-align: center;'><a title='Board setting.' href='"));
            					plen = fill_tcp_data_p(buf, plen, baseurl);
            					plen = fill_tcp_data_p(buf, plen, PSTR("'>back</a></p>"));
            					goto SENDTCP;
            				}
            			}

            			// Addr
            			if((strncmp("addr=", (char *) &(buf[i]), 5) == 0)) {	// parsing addr
            				for(j = 0; j < 4; j++) {
            					if((strncmp("&", (char *) &(buf[i+5+j]), 1) == 0)) {
            						memcpy(value, (unsigned char *) &(buf[i+5]), j+1);
            						break;
            					}
            				}
            				// send value of addr to the board
            				addrChar = atoi(value);
            				addrInt = (int)addrChar;
            				IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, addrChar);
            				signals = 0x01; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals); // write addr signal
            				signals = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals);

            				plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK"));
							goto SENDTCP;
            			}

            			// DATA
            			if((strncmp("data=", (char *) &(buf[i]), 5) == 0)) {	// parsing data
							for(j = 0; j < 4; j++) {
								if((strncmp("&", (char *) &(buf[i+5+j]), 1) == 0)) {
									memcpy(value, (unsigned char *) &(buf[i+5]), j+1);
									break;
								}
							}
							wdata = atoi(value);
							wdataInt = (int)wdata;
							// send value of data to the board
							IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, wdata);
							signals = 0x08; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals); // write data signal
							signals = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals);

							plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK"));
							goto SENDTCP;
						}

            			// DAC
						if((strncmp("dac=", (char *) &(buf[i]), 4) == 0)) {	// parsing data
							for(j = 0; j < 32; j++) {
								if((strncmp("&", (char *) &(buf[i+4+j]), 1) == 0)) {
									memcpy(valuelong, (unsigned char *) &(buf[i+4]), j+1);
									break;
								}
							}
							addrTmp = addrChar;
							addrChar = 0x24;
							addrInt = (int)addrChar;
							IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, addrChar);
							signals = 0x01; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals); // write addr signal
							signals = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals);
							for(i = 0; i < delay; i++);

							wdataInt = atoi(valuelong);
							dacInt = atoi(valuelong);
							wdata = wdataInt;
							IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, wdata);
							signals = 0x08; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals); // write data signal
							signals = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals);
							for(i = 0; i < delay; i++);

							addrChar = 0x25;
							addrInt = (int)addrChar;
							IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, addrChar);
							signals = 0x01; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals); // write addr signal
							signals = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals);
							for(i = 0; i < delay; i++);

							wdataInt =atoi(valuelong) >> 8;
							wdata = wdataInt;
							IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, wdata);
							signals = 0x08; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals); // write data signal
							signals = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals);
							for(i = 0; i < delay; i++);

							addrChar = addrTmp;
							IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, addrChar);
							signals = 0x01; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals); // write addr signal
							signals = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals);

							// check "dac write signal"
							if(iBuf < 200)
								iBuf = iBuf + 1;
							if(iBuf == 200) {
								for(iiBuf = 1; iiBuf < iBuf; iiBuf++)
									dacBuf[iiBuf - 1] = dacBuf[iiBuf];
							}
							dacBuf[iBuf] = dacInt;

							plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK"));
							goto SENDTCP;
						}

            		}
            		plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 OK</h1>"));
            		goto SENDTCP;
            	}
            	plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 OK</h1>"));
            	goto SENDTCP;
            }

            // main page
            if(strncmp("/ ", (char *) &(buf[dat_p + 4]), 2) == 0) {
           	plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
           	plen = fill_tcp_data_p(buf, plen, PSTR(""
           			"<p style='text-align: center;'>JOINT INSTITUTE FOR NUCLEAR RESEARCH</p>"
            			"<p style='text-align: center;'>PMTLab WEBSCAN, version: "));
				addrChar = 0;
				addrInt = (int)addrChar;
				IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, addrChar);
				signals = 0x01; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals); // write addr signal
				signals = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals);
				rdata = 0x00; rdata = IORD_ALTERA_AVALON_PIO_DATA(PIO_RDATA_BASE);
				char version1 = rdata >> 4;
				char version2 = rdata << 4;
				version2 = version2 >> 4;
				char data_array[5];
				sprintf(data_array," %d.%d ",(int)version1, (int)version2);
				plen = fill_tcp_data_p(buf, plen, data_array);
				plen = fill_tcp_data_p(buf, plen, PSTR("</p>"));
				plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>support: <a href='mailto:pmtlab.jinr@gmail.com'>pmtlab.jinr@gmail.com</a></p>"));

				plen = fill_tcp_data_p(buf, plen, PSTR(""
							            			"<p style='text-align: center;'>Authorization:</p>"
							            			"<form action='/' method='POST'>"
							            			"<p style='text-align: center;'> Login:"
							            			"<input name='login' size='11' type='text' value='pmtlab' />"
							            			" &#20 Password:"
							            			"<input name='pass' size='8' type='password' value='amsams' />"
							            			"<input type='submit' value='OK' /> </p>"
							            			"</form>"));
            	goto SENDTCP;
            }
			 // setting
			 	 if(strncmp("/set1.html ", (char *) &(buf[dat_p + 4]), 9) == 0) {
			 		plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
			 		// generator
			 	/*	plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>SETTING</p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>GENERATOR:</p>"
																"<form id='idGEN'>"
																"<p style='text-align: center;'> freq:"
																"<input name='freq' size='8' type='text' value='000' />"
																"<input name='flag' size='8' type='hidden' value='flag' />"
																"<input type='submit' value='OK' />"
																"</form></p>")); */
					// gate
				/*	plen = fill_tcp_data_p(buf, plen, PSTR(""
															"<p style='text-align: center;'>GATE:</p>"
															"<form id='idGate'>"
															"<p style='text-align: center;'> gate:"
															"<input name='gate' size='8' type='text' value='000' />"
															"<input name='flag' size='8' type='hidden' value='flag' />"
															"<input type='submit' value='OK' />"
															"</p></form>")); */
					// dac
					plen = fill_tcp_data_p(buf, plen, PSTR(""
															"<p style='text-align: center;'>DAC:</p>"
															"<form id='idDac'>"
															"<p style='text-align: center;'> &#20 code:"
															"<input name='dac' size='8' type='text' value='0000' />"
															"<input name='flag' size='8' type='hidden' value='flag' />"
															"<input type='submit' value='OK' />"
															"</p></form>"));

					goto SENDTCP;
			 	 }
			 	 if(strncmp("/set2.html ", (char *) &(buf[dat_p + 4]), 9) == 0) {
					plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));

					// addr
					plen = fill_tcp_data_p(buf, plen, PSTR(""
															"<p style='text-align: center;'>ADDRESS:</p>"
															"<form id='idAddr'>"
															"<p style='text-align: center;'> addr:"
															"<input name='addr' size='8' type='text' value='000' />"
															"<input name='flag' size='8' type='hidden' value='flag' />"
															"<input type='submit' value='OK' /></p></form>"
															""));
					// mem
					plen = fill_tcp_data_p(buf, plen, PSTR(""
															"<p style='text-align: center;'>DATA:</p>"
															"<form id='idMemo'>"
															"<p style='text-align: center;'> data:"
															"<input name='data' size='8' type='text' value='000' />"
															"<input name='flag' size='8' type='hidden' value='flag' />"
															"<input type='submit' value='OK' /></p></form>"
															""));
					goto SENDTCP;
				 }
				 // status
				 if(strncmp("/gen.html ", (char *) &(buf[dat_p + 4]), 8) == 0) {
					plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<div id='main'>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>STATUS</p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: left;'>&#20</p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>freq: &#20 "));
					char gen_array[32];
					sprintf(gen_array,"%d",(int)showFreq);
					plen = fill_tcp_data_p(buf, plen, gen_array);
					plen = fill_tcp_data_p(buf, plen, PSTR(", Hz </p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("</div>"));
					goto SENDTCP;
				 }
				 if(strncmp("/gate.html ", (char *) &(buf[dat_p + 4]), 9) == 0) {
					plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<div id='main'>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: left;'>&#20</p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>gate: &#20 "));
				//	char addr_array[3];
				//	addrInt = (int)addrChar;
					sprintf(addrArray,"%d", addrInt);
					plen = fill_tcp_data_p(buf, plen, addrArray);
					plen = fill_tcp_data_p(buf, plen, PSTR("</p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("</div>"));
					goto SENDTCP;
				 }
				 if(strncmp("/dac.html ", (char *) &(buf[dat_p + 4]), 9) == 0) {
					plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<div id='main'>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: left;'>&#20</p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>code: &#20 "));
					sprintf(addrArray,"%d", dacInt);
					plen = fill_tcp_data_p(buf, plen, addrArray);
					plen = fill_tcp_data_p(buf, plen, PSTR("</p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("</div>"));
					goto SENDTCP;
				 }
				 // status
				 if(strncmp("/addr.html ", (char *) &(buf[dat_p + 4]), 9) == 0) {
					plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<div id='main'>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: left;'>&#20</p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>addr: &#20 "));
					addrChar = IORD_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE);
					addrInt = (int)addrChar;
					sprintf(addrArray,"%d", addrInt);
					plen = fill_tcp_data_p(buf, plen, addrArray);
					plen = fill_tcp_data_p(buf, plen, PSTR("</p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("</div>"));
					goto SENDTCP;
				 }
				 if(strncmp("/data.html ", (char *) &(buf[dat_p + 4]), 9) == 0) {
					plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<div id='main'>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: left;'>&#20</p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>data: &#20 "));
					unsigned char rdata = 0x00; rdata = IORD_ALTERA_AVALON_PIO_DATA(PIO_RDATA_BASE);
						unsigned char data_array[3];
						sprintf(data_array,"%d",(int)rdata);
					plen = fill_tcp_data_p(buf, plen, data_array);
					plen = fill_tcp_data_p(buf, plen, PSTR("</p>"));
					plen = fill_tcp_data_p(buf, plen, PSTR("</div>"));
					goto SENDTCP;
				 }
			 	 // live data
			 				 if(strncmp("/live-data ", (char *) &(buf[dat_p + 4]), 9) == 0) {
			 					signals = 0x02; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals); // read counter signal
			 					signals = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, signals);
			 					plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-type: text/json\r\n\r\n"));
			 					// write count
			 					count = 0; count = IORD_ALTERA_AVALON_PIO_DATA(PIO_COUNT_BASE);
			 					// write time (sec)
			 					time = 0; time = IORD_ALTERA_AVALON_PIO_DATA(PIO_TIME_BASE);

			 					// time (sec)
			 					live_time_count = live_time_count + (time*20)/1e9;
								int intpartLTime = (int)live_time_count;
								int fractpartLTime = (live_time_count - intpartLTime)*100000;

			 					// write freq (Hz)
			 					freq = (count/((time*20)/1e9))/2;
			 					int intpartFreq = (int)freq;
			 					int fractpartFreq = (freq - intpartFreq)*100000;

			 					char freq_array[32];
			 					sprintf(freq_array,"[%d.%d, %d.%d]", intpartLTime, fractpartLTime, intpartFreq, fractpartFreq);
			 					plen = fill_tcp_data_p(buf, plen, freq_array);

			 					// write freq (Hz)
			 					freq = 0;
			 					goto SENDTCP;
			 				 }
			 				 // dac buf value
							 if(strncmp("/live-dac ", (char *) &(buf[dat_p + 4]), 8) == 0) {
								 plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-type: text/json\r\n\r\n"));
								char array[5];
								for(iiBuf = 0; iiBuf < iBuf; iiBuf++) {
									sprintf(array,"%d;", (int)dacBuf[iiBuf]);
									plen = fill_tcp_data_p(buf, plen, array);
								}
								goto SENDTCP;
							 }
			  // freq
						 if(strncmp("/freq.html", (char *) &(buf[dat_p + 4]), 9) == 0) {
							live_time_count = 0;
							plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n"));
							plen = fill_tcp_data_p(buf, plen, PSTR(""
									"<html>"
									"<head>"
									"<script type='text/javascript'></script>"
									"<script type='text/javascript' src='http://code.jquery.com/jquery-2.2.4.js'></script>"
									"<script type='text/javascript' src='http://code.highcharts.com/highcharts.js'></script>"
									"<script type='text/javascript' src='http://192.168.1.4/counter.js'></script>"
									"</head>"
									"<body>"
									"<div id='count_chart'></div>"
									"</body>"
									"</html>"
							));
							goto SENDTCP;
						 }
			 // freq
			 if(strncmp("/pages.html", (char *) &(buf[dat_p + 4]), 10) == 0) {
				live_time_count = 0;
				plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n"));
				plen = fill_tcp_data_p(buf, plen, PSTR(""
						"<p style='text-align: left;'>Links:</p> "
						"<p style='text-align: left;'><a href='http://www.w3schools.com/html/'>Info</a></p>   "
						"<p style='text-align: left;'><a href='http://www.w3schools.com/html/'>Memory map</a></p> "
						"<p style='text-align: left;'><a href='http://www.w3schools.com/html/'>Instructions</a></p> "
						"<p>"
				));
				goto SENDTCP;
			 }
			 // freq
			 if(strncmp("/counter.js", (char *) &(buf[dat_p + 4]), 4) == 0) {
				 plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type:application/javascript\r\n\r\n"));
				 plen = fill_tcp_data_p(buf, plen, PSTR(""
					 "var chart;"
					 "var startTime = new Date().getTime();"
					 "function requestData() {"
						 "$.ajax({"
							 "url: 'http://192.168.1.4/live-data',"
							 "success: function(point) {"
								 "var series = chart.series[0],"
								 "shift = series.data.length > 300;"
								 "chart.series[0].addPoint(eval(point), true, shift);"
								 "setTimeout(requestData, 3000);"
							 "},"
							 "cache: false"
						 "});"
					 "}"
					 "$(document).ready(function() {"
						 "chart = new Highcharts.Chart({"
							 "chart: {"
								 "renderTo: 'chart',"
								 "defaultSeriesType: 'spline',"
								 "events: {"
									 "load: requestData"
								 "}"
							 "},"
							 "title: {"
								 "text: 'COUNTER'"
							 "},"
							 "xAxis: {"
						 	 	 "minPadding: 0.0,"
								 "maxPadding: 0.2,"
						 	 	 "title: {"
								 	 "text: 'time, sec',"
									 "margin: 80"
								 "}"
							 "},"
							 "yAxis: {"
								 "minPadding: 0.2,"
								 "maxPadding: 0.2,"
							 	 "title: {"
								 	 "text: 'freq, Hz',"
								 	 "margin: 80"
							 	 "}"
						 	 "},"
						 "series: [{"
							 "name: 'counter',"
							 "data: []"
						 "}]"
					 "});"
				"});"));
				 goto SENDTCP;
			 }
			 // freq
			 if(strncmp("/but1.js", (char *) &(buf[dat_p + 4]), 7) == 0) {
				 plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type:application/javascript\r\n\r\n"));
				 plen = fill_tcp_data_p(buf, plen, PSTR(""
						 "$( '#s1' ).load( 'http://192.168.1.4/gen.html #main' );"
						 "$( '#s2' ).load( 'http://192.168.1.4/gate.html #main' );"
						 "$( '#s3' ).load( 'http://192.168.1.4/dac.html #main' );"

						 "$('#set1').on('submit', '#idGEN', function(event) {"
							"event.preventDefault();"
						 "console.log( $( this ).serialize() );"
						 "var param = $( this ).serialize();"
						 "$.post('http://192.168.1.4/', param  );"
						 "$( '#s1' ).load( 'http://192.168.1.4/gen.html #main' );"
						 "});"

						 "$('#set1').on('submit', '#idGate', function(event) {"
							"event.preventDefault();"
						 "console.log( $( this ).serialize() );"
						 "var param = $( this ).serialize();"
						 "$.post('http://192.168.1.4/', param  );"
						 "$( '#s2' ).load( 'http://192.168.1.4/gate.html #main' );"
						 "});"

						 "$('#set1').on('submit', '#idDac', function(event) {"
							"event.preventDefault();"
						 "console.log( $( this ).serialize() );"
						 "var param = $( this ).serialize();"
						 "$.post('http://192.168.1.4/', param  );"
						 "$( '#s5' ).load( 'http://192.168.1.4/addr.html #main' );"
						 "$( '#s6' ).load( 'http://192.168.1.4/data.html #main' );"
						 "$( '#s3' ).load( 'http://192.168.1.4/dac.html #main' );"
						 "});"
				 ));
				 goto SENDTCP;
			 }
		 // freq
			 if(strncmp("/but2.js", (char *) &(buf[dat_p + 4]), 7) == 0) {
				 plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type:application/javascript\r\n\r\n"));
				 plen = fill_tcp_data_p(buf, plen, PSTR(""
						 "$( '#s5' ).load( 'http://192.168.1.4/addr.html #main' );"
						 "$( '#s6' ).load( 'http://192.168.1.4/data.html #main' );"

						 "$('#set2').on('submit', '#idAddr', function(event) {"
							"event.preventDefault();"
						 "console.log( $( this ).serialize() );"
						 "var param = $( this ).serialize();"
						 "$.post('http://192.168.1.4/', param  );"
						 "$( '#s5' ).load( 'http://192.168.1.4/addr.html #main' );"
						 "$( '#s6' ).load( 'http://192.168.1.4/data.html #main' );"
						 "});"

						 "$('#set2').on('submit', '#idMemo', function(event) {"
							"event.preventDefault();"
						 "console.log( $( this ).serialize() );"
						 "var param = $( this ).serialize();"
						 "$.post('http://192.168.1.4/', param  );"
						 "$( '#s6' ).load( 'http://192.168.1.4/data.html #main' );"
						 "$( '#s1' ).load( 'http://192.168.1.4/gen.html #main' );"
						 "});"
				 ));
				 goto SENDTCP;
			 }
			 // main css
			 if(strncmp("/basic.css", (char *) &(buf[dat_p + 4]), 4) == 0) {
				plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-type: text/css\r\n\r\n"));
				plen = fill_tcp_data_p(buf, plen, PSTR(""
						"body{"
						"background: #f4f4f4;"
						"}"
						"#wrapper{"
						"width: 100%;"
						"height: 100vh;"
						"}"
						"#header{"
						"width: 100%;"
						"height: 3%;"
						"}"
						"#center_wrap{"
						"width: 96%;"
						"height: 90%;"
						 "margin: 0 auto;"
						 "margin-top: 20px;"
						"background: #ffffff;"
						"box-shadow: 0 0 5px 1px;"
						"}"
						"#chart{"
						"width: 60%;"
						"height: 100%;"
						"float: left;"
						"background: #ffffff;"
						"}"
						"#set_stat{"
						"width: 40%;"
						"height: 100%;"
						"float: left;"
						"background: #ffffff;"
						"}"
						"#status{"
						"width: 50%;"
						"height: 100%;"
						"float: left;"
						"background: #ffffff;"
						"}"
						"#setting{"
						"width: 50%;"
						"height: 100%;"
						"float: left;"
						"background: #ffffff;"
						"}"
						"#foo{"
						"width: 100%;"
						"height:7%;"
						"}"
				));

				goto SENDTCP;
			 }
            cmd = analyse_get_url((char *) &(buf[dat_p + 5]));

            // for possible status codes see:
            if(cmd == (unsigned char)-1) {
            	plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
            	goto SENDTCP;
            }
			
            SENDTCP:
            make_tcp_ack_from_any(buf); // send ack for http get
            make_tcp_ack_with_data(buf, plen); // send data
            continue;
		   }
	   }

      // tcp port www end
      // udp start, we listen on udp port 1200=0x4B0
	   if(buf[IP_PROTO_P] == IP_PROTO_UDP_V && buf[UDP_DST_PORT_H_P] == 4 && buf[UDP_DST_PORT_L_P] == 0xb0) {
		   payloadlen = buf[UDP_LEN_L_P] - UDP_HEADER_LEN;
		   // you must sent a string starting with v
		   // e.g udpcom version 10.0.0.24
		   if(verify_password((char *) &(buf[UDP_DATA_P]))) {
			   // find the first comma which indicates
			   // the start of a command:
			   cmd_pos = 0;
			   while(cmd_pos < payloadlen) {
				   cmd_pos++;
				   if(buf[UDP_DATA_P + cmd_pos] == ',') {
					   cmd_pos++; // put on start of cmd
					   break;
				   }
			   }
			   // a command is one char and a value. At
			   // least 3 characters long. It has an '=' on
			   // position 2:
			   if(cmd_pos < 2 || cmd_pos > payloadlen - 3 || buf[UDP_DATA_P + cmd_pos + 1] != '=') {
				   strcpy(str, "e=no_cmd");
				   goto ANSWER;
			   }
			   // supported commands are
			   // t=1 t=0 t=?
			   if(buf[UDP_DATA_P + cmd_pos] == 't') {
				   cmdval = buf[UDP_DATA_P + cmd_pos + 2];
				   if(cmdval == '1') {
					   strcpy(str, "t=1");
					   goto ANSWER;
				   }
				   else if(cmdval == '0') {
					   strcpy(str, "t=0");
					   goto ANSWER;
				   }
				   else if(cmdval == '?') {
					   strcpy(str, "t=0");
					   goto ANSWER;
				   }
			   }
			   strcpy(str, "e=no_such_cmd");
			   goto ANSWER;
		   }
		   strcpy(str, "e=invalid_pw");
		   ANSWER: make_udp_reply_from_request(buf, str, strlen(str), myudpport);
	   }
   }
   return (0);
}
