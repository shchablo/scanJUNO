//------------------------------------------------------------------------------
// Copyright [2016] [Shchablo Konstantin]

//------------------------------------------------------------------------------
// Information.
// Company: JINR PMTLab
// Author: Shchablo Konstantin
// Email: ShchabloKV@gmail.com
// Tel: 8-906-796-76-53 (russia)
//-------------------------------------------------------------------------------

module counter
#(
	parameter DATA_WIDTH=8, parameter ADDR_WIDTH=10
 )
 (
	input wire clk,
	input wire [DATA_WIDTH-1:0]addr,
	input wire [DATA_WIDTH-1:0]data_in, output wire [DATA_WIDTH-1:0]data_out,
	input wire we, input wire initialization, input wire res,
	
	
	output wire [31:0]counter_time_ex,
	output wire [31:0]data_ex,
	
	input wire start,
	output wire stop,
	
	input wire signal,
	
	output wire dwrite
);

(* syn_encoding = "safe" *) reg [3:0] state; 

parameter init = 0, idle = 1, write = 2, counter = 3, delay = 4, end_counter = 5;

reg [DATA_WIDTH-1:0]ram[ADDR_WIDTH-1:0];

reg [31:0]count_time;
reg [31:0]sec;
reg [31:0]data;
reg [7:0]secTime;

assign counter_time_ex[7:0] = ram[6];
assign counter_time_ex[15:8] = ram[7];
assign counter_time_ex[23:16] = ram[8];
assign counter_time_ex[31:24] = ram[9];

assign data_ex[7:0] = ram[2];
assign data_ex[15:8] = ram[3];
assign data_ex[23:16] = ram[4];
assign data_ex[31:24] = ram[5];

assign dwrite = enable;

reg enable;
reg reset;
reg flag;
always @ (posedge signal or posedge reset) begin
	if(reset)
		data <= 0;
	else
		data <= data + enable;	

end
/*always @ (posedge clk) begin

	if(enable) begin
		if(signal) begin
			if(flag)
				count_time <= count_time + 1;
			flag <= 0;
		end
		else
			flag <= 1;
	end else begin
		count_time <= 0;		
	end
end */ 

always @ (posedge signal or posedge reset) begin
	if(reset)
		count_time <= 0;
	else 
		count_time <= count_time + 1;
end 

always @ (posedge clk) begin 
	case(state)
		init: begin
			if(res)
				state <= init;
			ram[0] <= 0;
			ram[1] <= 0;
		
			ram[2] <= 0;
			ram[3] <= 0;
			ram[4] <= 0;
			ram[5] <= 0;
		
			ram[6] <= 0;
			ram[7] <= 0;
			ram[8] <= 0;
			ram[9] <= 0;
			
			sec <= 0;
			secTime <= 0;
			
			stop <= 0;
			enable <= 0;
			state <= idle;
		end
		
		idle: begin
			enable <= 0;
			if(res || initialization || ram[0] == 8'h01)
				state <= init;
			if(start) begin
				sec <= 0;
				state <= delay;
			end
			if(we)
				state <= write;	
				
			 case(addr)
				8'h26:  data_out <= ram[0];
				8'h27:  data_out <= ram[1];
	  
				8'h28:  data_out <= ram[2];
				8'h29:  data_out <= ram[3];
				8'h30:  data_out <= ram[4];
				8'h31:  data_out <= ram[5];
	  
				8'h32:  data_out <= ram[6];
				8'h33:  data_out <= ram[7];
				8'h34:  data_out <= ram[8];
				8'h35:  data_out <= ram[9];	
			endcase
		end
		
		write: begin
			if(res)
				state <= init;
			case(addr)
				8'h26:  ram[0] <= data_in;
				8'h27:  ram[1] <= data_in;
				
				8'h28:  ram[2] <= data_in;
				8'h29:  ram[3] <= data_in;
				8'h30:  ram[4] <= data_in;
				8'h31:  ram[5] <= data_in;
			 
				8'h32:  ram[6] <= data_in;
				8'h33:  ram[7] <= data_in;
				8'h34:  ram[8] <= data_in;
				8'h35:  ram[9] <= data_in;			
			endcase
			state <= idle;		
		end
		delay: begin
			if(res)
				state <= init;
			if(sec == 49998)
				reset <= 1;
			if(sec == 49999) begin
				reset <= 0;
				enable <= 1;	
				sec <= 0;
				secTime <= ram[1];
				stop <= 0;
				state <= counter;
			end
			else begin
				sec <= sec + 1;
			end
		end
		counter: begin
			if(res)
				state <= init;
			enable <= 1;	
	   	if(secTime > 0) begin
				if(sec == 49999999) begin
				//if(sec == 499999) begin
					secTime <= secTime - 1;
					sec <= 0;
				end
				else begin
					sec <= sec + 1;
				end
			end
			else begin
				enable <= 0;
				sec <= 0;
				stop <= 1;
				
				ram[2] <= data[7:0];
				ram[3] <= data[15:8];
				ram[4] <= data[23:16];
				ram[5] <= data[31:24];
			  
				ram[6] <= count_time[7:0];
				ram[7] <= count_time[15:8];
				ram[8] <= count_time[23:16];
				ram[9] <= count_time[31:24];
	
				state <= end_counter;
			end
		end
		end_counter : begin
			if(res)
				state <= init;
			enable <= 0;
			if(sec == 500000) begin
				stop <= 0;
				state <= idle;
			end
			else begin
				sec = sec + 1;
			end
		end
	endcase
end

endmodule
