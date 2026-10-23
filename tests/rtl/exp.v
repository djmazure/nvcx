`timescale 1 ns / 1 ps
module exp(input clk, input [7:0] a, b, output [7:0] y_comb, output reg [7:0] q, output [7:0] cnt_out);
  reg clk_int = 0;
  always #5 clk_int = ~clk_int;
  reg [7:0] cnt = 0;
  reg [7:0] q_i = 0;
  reg [7:0] mem [0:3];
  assign y_comb = a ^ b;
  always @(posedge clk) begin q_i <= a; mem[0] <= a; end
  always @* q = q_i;
  always @(posedge clk_int) cnt <= cnt + 1;
  assign cnt_out = cnt;
endmodule
