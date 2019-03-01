//------------------------------------------------------------------------------
// Copyright [2016] [Shchablo Konstantin]

//------------------------------------------------------------------------------
// Information.
// Company: JINR PMTLab
// Author: Shchablo Konstantin
// Email: ShchabloKV@gmail.com
// Tel: 8-906-796-76-53 (russia)
//-------------------------------------------------------------------------------

module selector(
 input [7:0]addr,
 input [7:0]gate,
 input [7:0]counter,
 input [7:0]pwm,
 input [7:0]version,
 input [7:0]dac,
 output reg [7:0]data
);

always @*
begin
  case(addr)
   8'h00:  data = version; 
	
   8'h20:  data = gate;
   8'h21:  data = gate;
   8'h22:  data = gate;
   
	8'h02:  data = dac;
	8'h03:  data = dac;
   8'h23:  data = dac;
   8'h24:  data = dac;
   8'h25:  data = dac;
	8'h47:  data = dac;
   
   8'h26:  data = counter;
   8'h27:  data = counter;
   8'h28:  data = counter;
   8'h29:  data = counter;
   8'h30:  data = counter;
   8'h31:  data = counter;
   8'h32:  data = counter;
   8'h33:  data = counter;
   8'h34:  data = counter;
   8'h35:  data = counter;
   
   8'h36:  data = pwm;
   8'h37:  data = pwm;
   8'h38:  data = pwm;
   8'h39:  data = pwm;
   8'h40:  data = pwm;
   8'h41:  data = pwm;
   8'h42:  data = pwm;
   8'h43:  data = pwm;
   8'h44:  data = pwm;
   8'h45:  data = pwm;
	8'h46:  data = pwm;	
  default:
  	data = 0;
  endcase
end

endmodule
