//------------------------------------------------------------------------------
// Copyright [2016] [Shchablo Konstantin]

//------------------------------------------------------------------------------
// Information.
// Company: JINR PMTLab
// Author: Shchablo Konstantin
// Email: ShchabloKV@gmail.com
// Tel: 8-906-796-76-53 (russia)
//-------------------------------------------------------------------------------

module gate
#(
	parameter DATA_WIDTH=8, parameter ADDR_WIDTH=3, 
	parameter OUT_WIDTH=4, 
	parameter OUT_LEDS=10
 )
 (
	input wire clk,
	input wire [DATA_WIDTH-1:0]addr,
	input wire [DATA_WIDTH-1:0]data_in, output wire [DATA_WIDTH-1:0]data_out,
	input wire we, input wire init, input wire res,
 
	output wire [OUT_WIDTH-1:0]out,
	
	input wire [OUT_LEDS-1:0]keys,
	output reg [OUT_LEDS-1:0]leds,
	
	input wire gen_in,
	output reg [OUT_LEDS-1:0]ch_pwm_out,
	output reg gen_out
);

reg [DATA_WIDTH-1:0]ram[ADDR_WIDTH-1:0]; // Addr: 8'h20. ram[0] - configure commands.
																				 //	8'b00000001 - reset. All ram memory = 0.
																				 // Addr: 8'h21. ram[1] - number last error:
																				 // 8'b00000001 - not correct gate number. Available [0;10].
																				 // Addr: 8'h22 ram[2][3:0] - gate number  
assign out = ram[2][3:0]; // output sigmal
always @ (posedge clk) begin 
	
	// initialization
	if(init) begin
		ram[0] <= 0;
		ram[1] <= 0;
		ram[2] <= 0;
		leds <= 0;
	end
	
  // write memory	
	if(we) begin
		case(addr)
			8'h20:	ram[0] <= data_in;
			8'h21:  ram[1] <= data_in;
			8'h22:  ram[2] <= data_in;
		endcase
	end
		
	// read memory  			
	case(addr)
		8'h20:	data_out <= ram[0];
		8'h21:  data_out <= ram[1];
	  8'h22:  data_out <= ram[2];
	endcase
	
/*	// decoder for leds (Altera Cyclone III DE0 board)
	case(out)
		4'd0:  leds <= 10'b0000000000;
		4'd1:  leds <= 10'b0000000001;
		4'd2:  leds <= 10'b0000000010;
		4'd3:  leds <= 10'b0000000100;
		4'd4:  leds <= 10'b0000001000;
		4'd5:  leds <= 10'b0000010000;
		4'd6:  leds <= 10'b0000100000;
		4'd7:  leds <= 10'b0001000000;
		4'd8:  leds <= 10'b0010000000;
		4'd9:  leds <= 10'b0100000000;
		4'd10: leds <= 10'b1000000000;
	default:
		leds <= 10'b0000000000;
	endcase */
		leds <= 10'b0000000000;
	
		if(out == 4'd0) begin
			ch_pwm_out[0] = 0;
			gen_out = 0;
		end
		if(out == 4'd1) begin
			ch_pwm_out[0] = gen_in;
			gen_out = gen_in;
		end
		if(out == 4'd2) begin
			ch_pwm_out[1] = gen_in;
			gen_out = gen_in;
		end
		if(out == 4'd3) begin
			ch_pwm_out[2] = gen_in;
			gen_out = gen_in;
		end
		if(out == 4'd4) begin
			ch_pwm_out[3] = gen_in;
			gen_out = gen_in;
		end
		if(out == 4'd5) begin
			ch_pwm_out[4] = gen_in;
			gen_out = gen_in;
		end
		if(out == 4'd6) begin
			ch_pwm_out[5] = gen_in;
			gen_out = gen_in;
		end
		if(out == 4'd7) begin
			ch_pwm_out[6] = gen_in;
			gen_out = gen_in;
		end
		if(out == 4'd8) begin
			ch_pwm_out[7] = gen_in;
			gen_out = gen_in;
		end
		if(out == 4'd9) begin
			ch_pwm_out[8] = gen_in;
			gen_out = gen_in;
		end
		if(out == 4'd10) begin
			ch_pwm_out[9] = gen_in;
			gen_out = gen_in;
		end
		if(out == 4'd11) begin
			gen_out = gen_in;
		end
		
		
	
	// Command: Reset 
	if(ram[0] == 8'b00000001 || res) begin
		ram[0] <= 0;
		ram[1] <= 0;
		ram[2] <= 0;
		leds <= 0;
	end
	
end
endmodule
