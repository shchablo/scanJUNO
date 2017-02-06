module eth_top(
		
		input  wire  reset_n,                   //             clk_clk_in_reset.reset_n
		output wire  out_port_from_the_LAN_RST, //  LAN_RST_external_connection.export
		output wire  out_port_from_the_LAN_CS,  //   LAN_CS_external_connection.export
		input  wire  MISO_to_the_LAN,           //                 LAN_external.MISO
		output wire  MOSI_from_the_LAN,         //                             .MOSI
		output wire  SCLK_from_the_LAN,         //                             .SCLK
		
		input  wire  clk,                       //                   clk_clk_in.clk
		
		input  wire  in_port_to_the_LAN_NINT,   // LAN_NINT_external_connection.export
		
		input  wire [31:0] time_export,               //                         time.export
		input  wire [31:0] signals_export,            //                      signals.export
		output wire [7:0]  addr_export,               //                         addr.export
		input  wire [7:0]  rdata_export,              //                        rdata.export
		
		output wire [7:0]  wdata_export,              //                        wdata.export
		output wire        swrite_export,             //                       swrite.export
		output wire        sread_export,              //                        sread.export
		
		output wire        cread_export,               //                        cread.export
		output wire        addr_write_export               //                        cread.export
	);

wire  SS_n_from_the_LAN;
wire  dclk_from_the_epcs;       //                epcs_external.dclk
wire  sce_from_the_epcs;         //                             .sce
wire  sdo_from_the_epcs;         //                             .sdo
wire  data0_to_the_epcs;         //                             .data0

wire        [7:0]signals_export_array;             //                       swrite.export
		

assign swrite_export = signals_export_array[3];
assign sread_export = signals_export_array[2];
assign cread_export = signals_export_array[1];
assign addr_write_export = signals_export_array[0];
sopc sopc_inst (
		
		reset_n,                   //             clk_clk_in_reset.reset_n
		
		out_port_from_the_LAN_RST, //  LAN_RST_external_connection.export
		out_port_from_the_LAN_CS,  //   LAN_CS_external_connection.export
		MISO_to_the_LAN,           //                 LAN_external.MISO
		MOSI_from_the_LAN,         //                             .MOSI
		SCLK_from_the_LAN,         //                             .SCLK
		SS_n_from_the_LAN,         //                             .SS_n
		
		clk,                       //                   clk_clk_in.clk
		in_port_to_the_LAN_NINT,   // LAN_NINT_external_connection.export
		
		dclk_from_the_epcs,        //                epcs_external.dclk
		sce_from_the_epcs,         //                             .sce
		sdo_from_the_epcs,         //                             .sdo
		data0_to_the_epcs,
				
		time_export,               //                         time.export
		signals_export,            //                      signals.export
		addr_export,               //                         addr.export
		rdata_export,              //                        rdata.export
		
		wdata_export,              //                        wdata.export
		
		signals_export_array
);

endmodule