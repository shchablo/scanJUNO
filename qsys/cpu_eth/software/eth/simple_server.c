/*******************************************************************************
* Copyright [2016] [Guido Socher (GPL V2), Shchablo Konstantin]
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
* either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/*******************************************************************************
* Information.
* Company: JINR PMTLab
* Author: Shchablo Konstantin
* Email: ShchabloKV@gmail.com
* Tel: 8-906-796-76-53 (russia)
*******************************************************************************/

#include "simple_server.h"
#include "math.h"
#define PSTR(s) s

static unsigned char mymac[6] = { 0x54, 0x55, 0x58, 0x10, 0x00, 0x24 }; // mac
static unsigned char myip[4] = { 159, 93, 74, 142 }; // ip
static char baseurl[] = "http://159.93.74.142/"; // a DNS server (baseurl must end in "/"):
static unsigned int mywwwport = 80; // listen port for tcp/www (max range 1-254)
static unsigned int myudpport = 1200; // listen port for udp

#define BUFFER_SIZE 1500
unsigned char buf[BUFFER_SIZE + 1];
unsigned char bufUDP[BUFFER_SIZE + 1];

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

// send data
void sendRun(unsigned char dac1, unsigned char dac2,
            unsigned char cTimeChar, unsigned char *addrChar)
{
    unsigned char addrTmp = *addrChar;
    int i = 0;
    int delay = 50000; // ml sec.

    // write dac code
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, 0x24);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    for(i = 0; i < delay; i++);

    IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, dac1);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x08); // write data signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    for(i = 0; i < delay; i++);

    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, 0x25);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    for(i = 0; i < delay; i++);

    IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, dac2);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x08); // write data signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    for(i = 0; i < delay; i++);

    // write  time
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE,  0x27);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    for(i = 0; i < delay; i++);

    IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, cTimeChar);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x08); // write data signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    for(i = 0; i < delay; i++);

    *addrChar = addrTmp;
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, *addrChar);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    for(i = 0; i < delay; i++);

}

void readDAC(int *dacInt, unsigned char *addrChar)
{
    unsigned char addrTmp = *addrChar;
    int i = 0;
    int delay = 50000; // ml sec.
    unsigned char memoDAC[2] = {0};

    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, 0x24);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    for(i = 0; i < delay; i++);

    memoDAC[0] = IORD_ALTERA_AVALON_PIO_DATA(PIO_RDATA_BASE);
    for(i = 0; i < delay; i++);

    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, 0x25);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    for(i = 0; i < delay; i++);

    memoDAC[1] = IORD_ALTERA_AVALON_PIO_DATA(PIO_RDATA_BASE);
    for(i = 0; i < delay; i++);

    *addrChar = addrTmp;
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, *addrChar);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);

    *dacInt |= (memoDAC[0]);
    *dacInt |= (memoDAC[1]<<8);
}

void sendData(unsigned char *wdataChar, int *wdataInt, unsigned char *value)
{
    *wdataChar = atoi(value);
    *wdataInt = atoi(value);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, *wdataChar);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x08); // write data signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
}

void sendAddr(unsigned char *addrChar, int *addrInt, unsigned char *value)
{
    *addrChar = atoi(value);
    *addrInt = atoi(value);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, *addrChar);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
}

void sendFreq(unsigned int *freqInt, unsigned char *value, unsigned char *addrChar)
{
    int i = 0;
    int delay = 50000; // ml sec.
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, 0x46);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    for(i = 0; i < delay; i++);

    *freqInt = atoi(value);
    IOWR_ALTERA_AVALON_PIO_DATA(DATA32_BASE, *freqInt);
    for(i = 0; i < delay; i++);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x20);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);

    for(i = 0; i < delay; i++);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, *addrChar);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);

}

void sendGate(unsigned int *gateInt, unsigned char *value, unsigned char *addrChar)
{
    int i = 0;
    int delay = 50000; // ml sec.
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, 0x22);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    for(i = 0; i < delay; i++);

    *gateInt = atoi(value);
    unsigned char str =  atoi(value);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, str);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x08); // write data signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
     for(i = 0; i < delay; i++);

    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, *addrChar);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);

}

// parsing
void parsRun(unsigned int ii,
             int *dacInt, unsigned char *dac1, unsigned char *dac2,
             int *cTime, unsigned char *cTimeChar,
             int *step, int *nSteps, unsigned char *calibration)
{
    int dacTmp = 0;
    unsigned char valuelong[32] = {0};
    unsigned int i, j, k, n, r, z;
    i = ii;
        for(j = 0; j < 32; j++) {
            if((strncmp("&", (char *) &(buf[i+4+j]), 1) == 0)) {
                memcpy(valuelong, (unsigned char *) &(buf[i+4]), j+1);
                *dacInt = atoi(valuelong);
                dacTmp = atoi(valuelong);
                *dac1 =  dacTmp;
                *dac2 =  dacTmp >> 8;
                break;
            }
        }
        for(k = 0; k < 32; k++) {
            if((strncmp("&", (char *) &(buf[i+4+j+6+k]), 1) == 0)) { // parsing time for counter
                memcpy(valuelong, (unsigned char *) &(buf[i+4+j+6]), k+1);
                *cTime = atoi(valuelong);
                *cTimeChar = *cTime;
                break;
            }
        }
        for(n = 0; n < 32; n++) {
            if((strncmp("&", (char *) &(buf[i+4+j+6+k+6+n]), 1) == 0)) { // parsing time for counter
                memcpy(valuelong, (unsigned char *) &(buf[i+4+j+6+k+6]), n+1);
                *step = atoi(valuelong);
                break;
            }
        }
        for(r = 0; r < 32; r++) {
            if((strncmp("&", (char *) &(buf[i+4+j+6+k+6+n+8+r]), 1) == 0)) { // parsing time for counter
                memcpy(valuelong, (unsigned char *) &(buf[i+4+j+6+k+6+n+8]), r+1);
                *nSteps = atoi(valuelong);
                break;
            }
        }
        for(z = 0; z < 32; z++) {
           if((strncmp("&", (char *) &(buf[i+4+j+6+k+6+n+8+r+6+z]), 1) == 0)) { // parsing time for counter
               memcpy(valuelong, (unsigned char *) &(buf[i+4+j+6+k+6+n+8+r+6]), z+1);
               *calibration = atoi(valuelong);
               break;
           }
       }
}

void pars(unsigned int ii, unsigned char *value)
{
    unsigned int i,j;
    i = ii;
    for(j = 0; j < 32; j++) {
        if((strncmp("&", (char *) &(buf[i+5+j]), 1) == 0)) {
            memcpy(value, (unsigned char *) &(buf[i+5]), j+1);
            break;
        }
    }
}

// main page

void addr_memo(unsigned int *len)
{
    *len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));

    // addr
    *len = fill_tcp_data_p(buf, *len, PSTR("<p style='text-align: center;'>ADDRESS:</p>"
        "<form id='idAddr'>"
            "<p style='text-align: center;'> addr:"
            "<input name='addr' size='8' type='text' value='000' />"
            "<input name='flag' size='8' type='hidden' value='flag' />"
            "<input type='submit' value='OK' /></p>"
        "</form>"));
    // memo
    *len = fill_tcp_data_p(buf, *len, PSTR("<p style='text-align: center;'>DATA:</p>"
        "<form id='idMemo'>"
            "<p style='text-align: center;'> data:"
            "<input name='data' size='8' type='text' value='000' />"
            "<input name='flag' size='8' type='hidden' value='flag' />"
            "<input type='submit' value='OK' /></p>"
        "</form>"));
}

void freq_gate(unsigned int *len)
{
    *len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));

    // freq
    *len = fill_tcp_data_p(buf, *len, PSTR("<p style='text-align: center;'>FREQ:</p>"
        "<form id='idFreq'>"
            "<p style='text-align: center;'> freq:"
            "<input name='freq' size='8' type='text' value='0' />"
            "<input name='flag' size='8' type='hidden' value='flag' />"
            "<input type='submit' value='OK' /></p>"
        "</form>"));
    // gate
    *len = fill_tcp_data_p(buf, *len, PSTR("<p style='text-align: center;'>GATE:</p>"
        "<form id='idGate'>"
            "<p style='text-align: center;'> gate:"
            "<input name='gate' size='8' type='text' value='0' />"
            "<input name='flag' size='8' type='hidden' value='flag' />"
            "<input type='submit' value='OK' /></p>"
        "</form>"));
}

void dacTR1(unsigned int *len)
{
    *len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));

    // RUN
    *len = fill_tcp_data_p(buf, *len, PSTR("<p style='text-align: center;'>CONFIGURATION:</p>"
        "<form id='idDacTR1'>"
            "<p style='text-align: center;'> code:"
            "<input name='dac' size='8' type='text' value='0000' /></p>"
            "<p style='text-align: center;'> time:"
            "<input name='time' size='8' type='text' value='000' />"
            "<p style='text-align: center;'> code:"
            "<input name='step' size='8' type='text' value='0' /></p>"
            "<p style='text-align: center;'> runs:"
            "<input name='nSteps' size='8' type='text' value='0' /></p>"
            "<p style='text-align: center;'> calibration:"
            "<input name='calb' input type='checkbox' value='1' /></p>"
            "<p style='text-align: center;'>"
            "<input name='flag' size='8' type='hidden' value='flag' /></p>"
            "<p style='text-align: center;'>"
            "<input type='submit' value='RUN' /></p>"
        "</form>"
        "<form id='idInterrupt'>"
            "<p style='text-align: center;'>"
            "<input type='submit' value='interrupt' />"
            "<input name='intr' size='8' type='hidden' value='flag' /></p>"
        "</form>"));
}

void basic_css(unsigned int *len)
{
    *len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-type: text/css\r\n\r\n"));
    *len = fill_tcp_data_p(buf, *len, PSTR("body{background: #f4f4f4;}"
        "#wr{width: 100%; height: 100vh;}" // wrapper
        "#h{width: 100%; height: 3%;}" // header
        "#c{width: 96%; height: 90%; margin: 0 auto; margin-top: 20px; background: #ffffff; box-shadow: 0 0 5px 1px;}" // center
        "#chart{width: 60%; height: 100%; float: left; background: #ffffff;}"
        "#st{width: 40%; height: 100%; float: left; background: #ffffff;}" // set_status
        "#s{width: 50%; height: 100%; float: left; background: #ffffff;}" // status
        "#foo{ width: 100%; height:7%; }"));
}

void amf(unsigned int *len)
{
    *len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type:application/javascript\r\n\r\n"));
    *len = fill_tcp_data_p(buf, *len, PSTR("$( '#a' ).load( 'http://159.93.74.142/addr.html #main' );"
                                           "$( '#m' ).load( 'http://159.93.74.142/memo.html #main' );"

        "$('#aM').on('submit', '#idAddr', function(event) {"
            "event.preventDefault();"
            "console.log( $( this ).serialize() );"
            "var param = $( this ).serialize();"
            "$.post('http://159.93.74.142/', param  );"
            "$( '#a' ).load( 'http://159.93.74.142/addr.html #main' );"
            "$( '#m' ).load( 'http://159.93.74.142/memo.html #main' );"
        "});"

        "$('#aM').on('submit', '#idMemo', function(event) {"
            "event.preventDefault();"
            "console.log( $( this ).serialize() );"
            "var param = $( this ).serialize();"
            "$.post('http://159.93.74.142/', param  );"
            "$( '#m' ).load( 'http://159.93.74.142/memo.html #main' );"
            "$( '#g' ).load( 'http://159.93.74.142/gate.html #main' );"
            "$( '#f' ).load( 'http://159.93.74.142/freq.html #main' );"
            "$( '#r' ).load( 'http://159.93.74.142/R1.html #main' );"
        "});"));
}

void fg(unsigned int *len)
{
    *len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type:application/javascript\r\n\r\n"));
    *len = fill_tcp_data_p(buf, *len, PSTR("$( '#f' ).load( 'http://159.93.74.142/freq.html #main' );"
                                           "$( '#g' ).load( 'http://159.93.74.142/gate.html #main' );"
        "$('#fG').on('submit', '#idFreq', function(event) {"
            "event.preventDefault();"
            "console.log( $( this ).serialize() );"
            "var param = $( this ).serialize();"
            "$.post('http://159.93.74.142/', param  );"
            "$( '#f' ).load( 'http://159.93.74.142/freq.html #main' );"
        "});"

        "$('#fG').on('submit', '#idGate', function(event) {"
            "event.preventDefault();"
            "console.log( $( this ).serialize() );"
            "var param = $( this ).serialize();"
            "$.post('http://159.93.74.142/', param  );"
            "$( '#g' ).load( 'http://159.93.74.142/gate.html #main' );"
        "});"));
}

void r1(unsigned int *len)
{
    *len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type:application/javascript\r\n\r\n"));
    *len = fill_tcp_data_p(buf, *len, PSTR("$( '#r' ).load( 'http://159.93.74.142/R1.html #main' );"
                                           "$( '#cM' ).load( 'http://159.93.74.142/counter.html' );"
    "$('#r1f').on('submit', '#idDacTR1', function(event) {"
        "event.preventDefault();"
        "console.log( $( this ).serialize() );"
        "var param = $( this ).serialize();"
        "$.post('http://159.93.74.142/', param  );"
        "$( '#r' ).load( 'http://159.93.74.142/R1.html #main' );"
        "$( '#cM' ).load( 'http://159.93.74.142/counter.html' );"
    "});"
    "$('#r1f').on('submit', '#idInterrupt', function(event) {"
       "event.preventDefault();"
       "console.log( $( this ).serialize() );"
       "var param = $( this ).serialize();"
       "$.post('http://159.93.74.142/', param  );"
       "$( '#r' ).load( 'http://159.93.74.142/R1.html #main' );"
    "});"));
}

void chart(unsigned int *len, int time)
{
    char str[64] = {0};
    sprintf(str, "setTimeout(requestData, %d);", time/2);
    *len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type:application/javascript\r\n\r\n"));
    *len = fill_tcp_data_p(buf, *len, PSTR("var chart;"
    "var startTime = new Date().getTime();"
    "function requestData() {"
        "$.ajax({"
            "url: 'http://159.93.74.142/live-data',"
            "success: function(point) {"
                "var series = chart.series[0],"
                "shift = series.data.length > 4096;"
                "chart.series[0].addPoint(eval(point), true, shift);"));
    sprintf(str, "setTimeout(requestData, %d);", time/2*1000);
    *len = fill_tcp_data_p(buf, *len, str);
    *len = fill_tcp_data_p(buf, *len, PSTR("},"
            "error: function() {"));
    *len = fill_tcp_data_p(buf, *len, str);
    *len = fill_tcp_data_p(buf, *len, PSTR("}"
        "});"
    "}"
    "$(document).ready(function() {"
        "chart = new Highcharts.Chart({"
            "chart: {"
                "renderTo: 'chart',"
                "defaultSeriesType: 'spline',"
                "events: {load: requestData}"
            "},"
            "title: {"));
    sprintf(str, "text: 'COUNTER, STEP TIME=%d sec'", time);
    *len = fill_tcp_data_p(buf, *len, str);
    *len = fill_tcp_data_p(buf, *len, PSTR("},"
            "xAxis: {"
                "minPadding: 0.0,"
                "maxPadding: 0.2,"
                "title: {"
                    "text: 'code',"
                    "margin: 80"
                "}"
            "},"
            "yAxis: {"
                "minPadding: 0.2,"
                "maxPadding: 0.2,"
                "title: {"
                    "text: 'count',"
                    "margin: 80"
                "}"
            "},"
            "series: [{"
                "name: 'count',"
                "data: []"
            "}]"
        "});"
    "});"
    "$('.chart-export').each(function() {"
        "var jThis = $(this),"
        "chartSelector = jThis.data('chartSelector'),"
        "chart = $(chartSelector).highcharts();"
        "$('*[data-type]', this).each(function() {"
            "var jThis = $(this),"
            "type = jThis.data('type');"
            "if(Highcharts.exporting.supports(type)) {"
                "jThis.click(function() {"
                    " chart.exportChartLocal({ type: type });"
                " });"
            "}"
            " else {"
                "jThis.attr('disabled', 'disabled');"
            "}"
        "});"
    "});"));
}

void main_page(unsigned int *len)
{
    *len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
    *len = fill_tcp_data_p(buf, *len, PSTR(""
        "<head>"
            "<link rel='stylesheet' href='http://159.93.74.142/bs.css' type='text/css' />"
            "<script type='text/javascript'></script>"
            "<script type='text/javascript' src='http://code.jquery.com/jquery-2.2.4.js'></script>"
            "<script type='text/javascript' src='http://code.highcharts.com/highcharts.js'></script>"
            "<script type='text/javascript' src='https://code.highcharts.com/modules/exporting.js'></script>"
            "<script type='text/javascript' src='http://highcharts.github.io/export-csv/export-csv.js'></script>"
        "</head>"
        "<body>"
            "<div id='wr'>" // wrapper
                "<div id='h'>""</div>" // header
                "<div id='c'>" // center
                    "<div id='cM'></div>" // chartMain
                    "<div id='st'>" // set status
                        "<div id='s'>" // status
                            "<div id='a'>""</div>" // addr
                            "<div id='m'>""</div>" // memo
                            "<div id='f'>""</div>" // freq
                            "<div id='g'>""</div>" // gate
                            "<div id='r'>""</div>" // run
                        "</div>"
                        // setting
                        "<div id='aM'></div>"
                        "<div id='fG'></div>"
                        "<div id='r1f'></div>"
                    "</div>"
                "</div>"
            "</div>"
            "<script>"
            "$('#aM').load('http://159.93.74.142/aM.html');"
            "$('#fG').load('http://159.93.74.142/fG.html');"
            "$('#r1f').load('http://159.93.74.142/R.html');"
            "</script>"
            "<script type='text/javascript' src='http://159.93.74.142/amf.js'></script>"
            "<script type='text/javascript' src='http://159.93.74.142/fg.js'></script>"
            "<script type='text/javascript' src='http://159.93.74.142/r1.js'></script>"
        "</body>"));
}

// start page
void start_page(unsigned int *len)
{
    *len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
    *len = fill_tcp_data_p(buf, *len, PSTR("<p style='text-align: center;'>JOINT INSTITUTE FOR NUCLEAR RESEARCH</p>"
                                               "<p style='text-align: center;'>PMTLab WEBSCAN, version: "));
    char addrChar = 0;
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, addrChar);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
    char rdata = 0x00; rdata = IORD_ALTERA_AVALON_PIO_DATA(PIO_RDATA_BASE);
    char version1 = rdata >> 4;
    char version2 = rdata << 4;
    version2 = version2 >> 4;
    char data_array[5];
    sprintf(data_array," %d.%d ",(int)version1, (int)version2);
    *len = fill_tcp_data_p(buf, *len, data_array);
    *len = fill_tcp_data_p(buf, *len, PSTR("</p>"));
    *len = fill_tcp_data_p(buf, *len, PSTR("<p style='text-align: center;'>support: <a href='mailto:pmtlab.jinr@gmail.com'>pmtlab.jinr@gmail.com</a></p>"));
    *len = fill_tcp_data_p(buf, *len, PSTR("<p style='text-align: center;'>Don't use webscan with udp!</p>"));

    *len = fill_tcp_data_p(buf, *len, PSTR(""
                                        "<p style='text-align: center;'>Authorization:</p>"
                                        "<form action='/' method='POST'>"
                                        "<p style='text-align: center;'> Login:"
                                        "<input name='login' size='11' type='text' value='pmtlab' />"
                                        " &#20 Password:"
                                        "<input name='pass' size='8' type='password' value='amsams' />"
                                        "<input type='submit' value='OK' /> </p>"
                                        "</form>"));
    *len = fill_tcp_data_p(buf, *len, PSTR(	"<p style='text-align: center;'><a href='http://www.w3schools.com/html/'>Setting</a></p>"));
}

// unauthorized page
void unauthorized(unsigned int *len)
{
    *len = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
    *len = fill_tcp_data_p(buf, *len, PSTR("<p style='text-align: center;'>Password isn't correct!</p>"));
    *len = fill_tcp_data_p(buf, *len, PSTR("<p style='text-align: center;'><a title='Board setting.' href='"));
    *len = fill_tcp_data_p(buf, *len, baseurl);
    *len = fill_tcp_data_p(buf, *len, PSTR("'>back</a></p>"));
}

int simple_server()
{
    char initialization = 1;
    if(initialization) {
        initialization = 0;
    }
    // common
    int delay = 50000;
    unsigned int plen;
    unsigned int dat_p;
    unsigned int i,j;
    char str[32] = {0};
    char result_array[32] = {0};
    unsigned char value[32] = {0};

    char waitRun = 0;
    char readLiveData = 0;
    int nSteps = 0;
    int step = 0;

    // time
    int cTime = 120; // sec
    unsigned char cTimeChar = 0;

    // dac
    int dacInt = 0;
    int dacTmp = 0;
    unsigned char dac1 = 0;
    unsigned char dac2 = 0;
    unsigned char calibration = 0;
    int calibrationInt = 0;

    // gate
    int gateInt = 0;

    // freq
    int freqInt = 0;

    // addr
    unsigned char addrChar = 0; IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, addrChar);
    int addrInt = 0;
    char addrArray[3] = {0};

    // data
    unsigned char wdataChar = 0x00; IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, wdataChar);
    int wdataInt = 0;

    unsigned char rdata = 0x00; rdata = IORD_ALTERA_AVALON_PIO_DATA(PIO_RDATA_BASE);
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);

    unsigned int count; count = IORD_ALTERA_AVALON_PIO_DATA(PIO_COUNT_BASE);
    unsigned int time; time = IORD_ALTERA_AVALON_PIO_DATA(PIO_TIME_BASE);

   /*initialize enc28j60*/
   enc28j60Init(mymac);

   str[0] = (char)enc28j60getrev();

   init_ip_arp_udp_tcp(mymac, myip, mywwwport);
   enc28j60PhyWrite(PHLCON, 0x476);
   enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz

   // init the ethernet/ip layer:
   while (1) {

       // RUN READ AND WAIT
       if(waitRun == 1 & (IORD_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_0_BASE) == 0x10)) {


           readLiveData = 1;
           IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x02); // read counter signal
           IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);

           // read count
           count = 0; count = IORD_ALTERA_AVALON_PIO_DATA(PIO_COUNT_BASE);
           // read time (sec)
           time = 0; time = IORD_ALTERA_AVALON_PIO_DATA(PIO_TIME_BASE);

         //  readDAC(&dacInt, &addrChar);

           if(count > 0 && calibration == 1) {
               calibrationInt = dacInt;
               // write dac code
               IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, 0x02);
               IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
               IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
               for(i = 0; i < delay; i++);

               IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, dac1);
               IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x08); // write data signal
               IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
               for(i = 0; i < delay; i++);

               IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, 0x03);
               IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
               IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
               for(i = 0; i < delay; i++);

               IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, dac2);
               IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x08); // write data signal
               IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
               for(i = 0; i < delay; i++);

              // IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, addrChar);
              // IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
              // IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);

               nSteps = 0;
               waitRun = 0;
               calibration = 0;

               sprintf(result_array,"[%d]", calibrationInt);

               for(i = 0; i < 1500; i++)
                   buf[i] = bufUDP[i];
               make_udp_reply_from_request(buf, result_array, strlen(result_array), myudpport);
           }
           else {
                 sprintf(result_array,"[%u, %u]", time, count);
              // sprintf(result_array,"[%d, %u]", dacInt, count);

               for(i = 0; i < 1500; i++)
                   buf[i] = bufUDP[i];
               make_udp_reply_from_request(buf, result_array, strlen(result_array), myudpport);

               if(nSteps > 0) {
                   nSteps = nSteps - 1;
                   waitRun = 1;

                   dacInt = dacInt + step;
                   dacTmp = dacInt;
                   dac1 =  dacTmp;
                   dac2 =  dacTmp >> 8;
                   dacTmp = 0;
                   sendRun(dac1, dac2, cTimeChar,  &addrChar);

                 //  IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, 0x23);
                 //  IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
                 //  IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
                 //  for(i = 0; i < delay; i++);

                 //  IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, 0x02);
                 //  IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x08); // write data signal
                 //  IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);

                   IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x10);
                   IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
               }
               else {
                   waitRun = 0;
               }
           }
        }

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
                        // parsing authorized post requst
                        if((strncmp("login=", (char *) &(buf[i]), 6) == 0)) {
                            if(strncmp("pmtlab&pass=amsams", (char *) &(buf[i+6]), 18) == 0) { // parsing login and password
                                main_page(&plen);
                                goto SENDTCP;
                            }
                            else {
                                unauthorized(&plen);
                                goto SENDTCP;
                            }
                        }
                        // parsing addr post requst
                        if((strncmp("addr=", (char *) &(buf[i]), 5) == 0)) {	// parsing addr
                            pars(i, value);
                            // send value of addr to the board
                            sendAddr(&addrChar, &addrInt, value);
                            plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK"));
                            goto SENDTCP;
                        }

                        // parsing data post requst
                        if((strncmp("data=", (char *) &(buf[i]), 5) == 0)) {	// parsing data
                            pars(i, value);
                            // send value of data to the board
                            sendData(&wdataChar, &wdataInt, value);
                            plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK"));
                            goto SENDTCP;
                        }
                        // parsing freq post requst
                        if((strncmp("freq=", (char *) &(buf[i]), 5) == 0)) {	// parsing freq
                            pars(i, value);
                            // send value of addr to the board
                            sendFreq(&freqInt, value, &addrChar);
                            plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK"));
                            goto SENDTCP;
                        }

                        // parsing data post requst
                        if((strncmp("gate=", (char *) &(buf[i]), 5) == 0)) {	// parsing data
                            pars(i, value);
                            // send value of data to the board
                            sendGate(&gateInt, value, &addrChar);
                            plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK"));
                            goto SENDTCP;
                        }
                        // parsing data post requst
                        if((strncmp("intr=", (char *) &(buf[i]), 5) == 0)) {	// parsing data
                            // send value of data to the board
                            nSteps = 0;
                            plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK"));
                            goto SENDTCP;
                        }

                        // parsing  run post requst
                        if((strncmp("dac=", (char *) &(buf[i]), 4) == 0)) {	// parsing dac data
                            // send run configuration
                                if(!waitRun) {
                                    parsRun(i, &dacInt, &dac1, &dac2, &cTime, &cTimeChar, &step, &nSteps, &calibration);
                                    waitRun = 1;
                                    sendRun(dac1, dac2, cTimeChar,  &addrChar);

                                  //  IOWR_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE, 0x23);
                                  //  IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x01); // write addr signal
                                  //  IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
                                  //  for(i = 0; i < delay; i++);

                                  //  IOWR_ALTERA_AVALON_PIO_DATA(PIO_WDATA_BASE, 0x02);
                                  // IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x08); // write data signal
                                  //  IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);

                                   IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x10);
                                   IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
                                    plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK"));
                                }
                                else {
                                    plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 WAIT</h1>"));
                                }
                            goto SENDTCP;
                        }

                    }
                    plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 OK</h1>"));
                    goto SENDTCP;
                }
                plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 OK</h1>"));
                goto SENDTCP;
            }

            // start page
            if(strncmp("/ ", (char *) &(buf[dat_p + 4]), 2) == 0) {
                start_page(&plen); // open star page
                goto SENDTCP;
            }
            // basic css page
            if(strncmp("/bs.css", (char *) &(buf[dat_p + 4]), 7) == 0) {
                basic_css(&plen); // open css page
                goto SENDTCP;
            }
            // chart page
            if(strncmp("/counter.js", (char *) &(buf[dat_p + 4]), 11) == 0) {
                chart(&plen, cTime);
                goto SENDTCP;
            }
            // chart page
            if(strncmp("/counter.html", (char *) &(buf[dat_p + 4]), 13) == 0) {
                plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
                plen = fill_tcp_data_p(buf, plen, PSTR("<script type='text/javascript' src='http://159.93.74.142/counter.js'> </script>"));
                plen = fill_tcp_data_p(buf, plen, PSTR("<div id='chart'></div>"));
                goto SENDTCP;
            }
            // live data
            if(strncmp("/live-data", (char *) &(buf[dat_p + 4]), 10) == 0) {
                if(!readLiveData) {
                    plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-type: text/json\r\n\r\n"));
                    goto SENDTCP;
                }
                plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-type: text/json\r\n\r\n"));

                plen = fill_tcp_data_p(buf, plen, result_array);
                readLiveData = 0;
                goto SENDTCP;
             }

            // parts of main page

                 // addrMemo
                 if(strncmp("/aM.html ", (char *) &(buf[dat_p + 4]), 8) == 0) {
                    addr_memo(&plen); // load forms
                    goto SENDTCP;
                 }
                 // dacT
                 if(strncmp("/R.html ", (char *) &(buf[dat_p + 4]), 7) == 0) {
                     dacTR1(&plen); // load form
                     goto SENDTCP;
                 }
                 // dacT
                 if(strncmp("/fG.html ", (char *) &(buf[dat_p + 4]), 8) == 0) {
                     freq_gate(&plen); // load form
                     goto SENDTCP;
                 }

            // js for status

                 // addr and memo function
                 if(strncmp("/amf.js", (char *) &(buf[dat_p + 4]), 6) == 0) {
                     amf(&plen);
                     goto SENDTCP;
                 }
                 // freq and gate function
                 if(strncmp("/fg.js", (char *) &(buf[dat_p + 4]), 5) == 0) {
                     fg(&plen);
                     goto SENDTCP;
                }
                 // run 1 function
                 if(strncmp("/r1.js", (char *) &(buf[dat_p + 4]), 5) == 0) {
                     r1(&plen);
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
                    plen = fill_tcp_data_p(buf, plen, PSTR("</p></div>"));
                    goto SENDTCP;
                 }
                 if(strncmp("/memo.html ", (char *) &(buf[dat_p + 4]), 9) == 0) {
                    plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<div id='main'>"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: left;'>&#20</p>"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>data: &#20 "));
                    rdata = IORD_ALTERA_AVALON_PIO_DATA(PIO_RDATA_BASE);
                    sprintf(str, "%d", (int)rdata);
                    plen = fill_tcp_data_p(buf, plen, str);
                    plen = fill_tcp_data_p(buf, plen, PSTR("</p></div>"));
                    goto SENDTCP;
                 }
                 // status
                 if(strncmp("/freq.html ", (char *) &(buf[dat_p + 4]), 9) == 0) {
                    plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<div id='main'>"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: left;'>&#20</p>"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>freq: &#20 "));
                    sprintf(str, "%d", freqInt);
                    plen = fill_tcp_data_p(buf, plen, str);
                    plen = fill_tcp_data_p(buf, plen, PSTR("</p></div>"));
                    goto SENDTCP;
                 }
                 if(strncmp("/gate.html ", (char *) &(buf[dat_p + 4]), 9) == 0) {
                    plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<div id='main'>"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: left;'>&#20</p>"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>gate: &#20 "));
                    sprintf(str, "%d", gateInt);
                    plen = fill_tcp_data_p(buf, plen, str);
                    plen = fill_tcp_data_p(buf, plen, PSTR("</p></div>"));
                    goto SENDTCP;
                 }

                 // dacR1
                 if(strncmp("/R1.html ", (char *) &(buf[dat_p + 4]), 6) == 0) {
                    plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<div id='main'>"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>RUN</p>"));
                    plen = fill_tcp_data_p(buf, plen, PSTR("<p style='text-align: center;'>dac: &#20 "));
                    sprintf(str,"%d", dacInt);
                    plen = fill_tcp_data_p(buf, plen, str);
                    plen = fill_tcp_data_p(buf, plen, PSTR("</p><p style='text-align: center;'>time: &#20 "));
                    sprintf(str,"%d", cTime);
                    plen = fill_tcp_data_p(buf, plen, str);
                    plen = fill_tcp_data_p(buf, plen, PSTR("</p><p style='text-align: center;'>step: &#20 "));
                    sprintf(str,"%d", step);
                    plen = fill_tcp_data_p(buf, plen, str);
                    plen = fill_tcp_data_p(buf, plen, PSTR("</p><p style='text-align: center;'>nRuns: &#20 "));
                    sprintf(str,"%d", nSteps);
                    plen = fill_tcp_data_p(buf, plen, str);
                    plen = fill_tcp_data_p(buf, plen, PSTR("</p><p style='text-align: center;'>calibration: &#20 "));
                    sprintf(str,"%d", calibrationInt);
                    plen = fill_tcp_data_p(buf, plen, str);
                    plen = fill_tcp_data_p(buf, plen, PSTR("</p></div>"));
                    goto SENDTCP;
                }

            // Page not found
            else {
                plen = fill_tcp_data_p(buf, 0, PSTR("HTTP/1.0 404 Page not found.\r\nContent-Type: text/html\r\n\r\n<h1>404 Page not found.</h1>"));
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
               for(i = 0; i < 500; i++) {
                   if(strncmp("hi", (char *) &(buf[i]), 2) == 0) {
                       for(j = 0; j < 1500; j++)
                           bufUDP[j] = buf[j];
                     strcpy(str, "hi");
                     goto ANSWER;
                   }
                   if(strncmp("help", (char *) &(buf[i]), 4) == 0) {
                     strcpy(str, "Information: '\n' "
                                 "/dac=[0-4095code]&time=[0-255sec]&step=[0-4095code]&nSteps=[0-4095]& '\n'"
                                 "/dac=[0-4095code]&time=[0-255sec]&step=[0-4095code]&nSteps=[0-4095]&calb&");
                     goto ANSWER;
                   }
                   if(strncmp("interrupt", (char *) &(buf[i]), 9) == 0) {
                      nSteps = 0;
                      strcpy(str, "nSteps = 0, Ok, please wait a few second.");
                      goto ANSWER;
                   }
                   if((strncmp("addr=", (char *) &(buf[i]), 5) == 0)) {	// parsing addr
                       pars(i, value);
                       // send value of addr to the board
                       sendAddr(&addrChar, &addrInt, value);
                       sprintf(str, "[%d]", addrInt);
                      goto ANSWER;
                   }
                   if((strncmp("raddr", (char *) &(buf[i]), 5) == 0)) {	// parsing addr
                        addrChar = IORD_ALTERA_AVALON_PIO_DATA(PIO_ADDR_BASE);
                        addrInt = (int)addrChar;
                        sprintf(addrArray,"%d", addrInt);
                        sprintf(str, "[%d]", addrInt);
                        goto ANSWER;
                   }
                   // parsing data post requst
                   if((strncmp("data=", (char *) &(buf[i]), 5) == 0)) {	// parsing data
                       pars(i, value);
                       // send value of data to the board
                       sendData(&wdataChar, &wdataInt, value);
                       sprintf(str, "[%d]", wdataInt);
                       goto ANSWER;
                   }
                   // parsing data post requst
                   if((strncmp("rdata", (char *) &(buf[i]), 5) == 0)) {	// parsing datas
                       rdata = IORD_ALTERA_AVALON_PIO_DATA(PIO_RDATA_BASE);
                       sprintf(str, "[%d]", (int)rdata);
                       goto ANSWER;
                   }
                   // parsing freq post requst
                   if((strncmp("freq=", (char *) &(buf[i]), 5) == 0)) {	// parsing freq
                       pars(i, value);
                       // send value of addr to the board
                       sendFreq(&freqInt, value, &addrChar);
                       sprintf(str, "[%d]", freqInt);
                       goto ANSWER;
                   }
                   // parsing freq post requst
                   if((strncmp("rfreq", (char *) &(buf[i]), 5) == 0)) {	// parsing freq
                       sprintf(str, "[%d]", freqInt);
                       goto ANSWER;
                   }
                   // parsing data post requst
                   if((strncmp("gate=", (char *) &(buf[i]), 5) == 0)) {	// parsing data
                       pars(i, value);
                       // send value of data to the board
                       sendGate(&gateInt, value, &addrChar);
                       sprintf(str, "[%d]", gateInt);
                       goto ANSWER;
                   }
                   // parsing freq post requst
                   if((strncmp("rgate", (char *) &(buf[i]), 5) == 0)) {	// parsing freq
                       sprintf(str, "[%d]", gateInt);
                       goto ANSWER;
                   }
                   if((strncmp("dac=", (char *) &(buf[i]), 4) == 0)) {	// parsing dac data
                       if(!waitRun) {
                           parsRun(i, &dacInt, &dac1, &dac2, &cTime, &cTimeChar, &step, &nSteps, &calibration);
                           waitRun = 1;
                           sendRun(dac1, dac2, cTimeChar,  &addrChar);
                           IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x10);
                           IOWR_ALTERA_AVALON_PIO_DATA(PIO_SIGNALS_BASE, 0x00);
                       }
                            else {
                                sprintf(str, "Please wait ~%d sec or interrupt run!", nSteps*cTime);
                                goto ANSWER;
                            }
                   strcpy(str, "Ok");
                   goto ANSWER;
                   }

               }
           strcpy(str, "No such command! help - for information about system.");
           ANSWER: make_udp_reply_from_request(buf, str, strlen(str), myudpport);
       }
   }
   return (0);
}
