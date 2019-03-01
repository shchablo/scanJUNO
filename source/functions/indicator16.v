//------------------------------------------------------------------------------
// Copyright [2016] [Shchablo Konstantin]

//------------------------------------------------------------------------------
// Information.
// Company: JINR PMTLab
// Author: Shchablo Konstantin
// Email: ShchabloKV@gmail.com
// Tel: 8-906-796-76-53 (russia)
//-------------------------------------------------------------------------------

module indicator16(
 input wire [3:0]code,
 output reg [7:0]segments
);

always @*
begin
  case(code)
   4'd0:  segments = 8'b11000000;
   4'd1:  segments = 8'b11111001;
   4'd2:  segments = 8'b10100100;
   4'd3:  segments = 8'b10110000;
   4'd4:  segments = 8'b10011001;
   4'd5:  segments = 8'b10010010;
   4'd6:  segments = 8'b10000010;
   4'd7:  segments = 8'b11111000;
   4'd8:  segments = 8'b10000000;
   4'd9:  segments = 8'b10010000;
   4'd10: segments = 8'b10001000;
   4'd11: segments = 8'b10000011;
   4'd12: segments = 8'b11000110;
   4'd13: segments = 8'b10100001;
   4'd14: segments = 8'b10000110;
   4'd15: segments = 8'b10001110;
  endcase
end

endmodule
