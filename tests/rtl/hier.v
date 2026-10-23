`timescale 1ns/1ps
module leaf #(parameter G = 1) (input clk, input [3:0] d, output reg [3:0] q);
  localparam K = G * 2;
  always @(posedge clk) q <= d;
endmodule
module hier #(parameter N = 2, W = 4) (input clk, input [W-1:0] din, output [W-1:0] dout, output integer ival, output real rval, output bval);
  wire [W-1:0] chain [0:N-1];
  reg [7:0] mem [0:3];
  typedef struct packed { logic a; logic [2:0] b; } rec_t;
  rec_t rec = '0;
  integer i_s = 5; real r_s = 1.5; bit b_s = 1; logic signed [7:0] sgn = -3; byte by = 7; int i32 = 9; longint i64 = 11; string str = "hi";
  genvar i;
  generate for (i = 0; i < N; i = i + 1) begin : g
    if (i == 0) begin : first leaf #(.G(i)) u (.clk(clk), .d(din), .q(chain[i])); end
    else begin : rest leaf #(.G(i)) u (.clk(clk), .d(chain[i-1]), .q(chain[i])); end
  end endgenerate
  assign dout = chain[N-1]; assign ival = i_s; assign rval = r_s; assign bval = b_s;
  always @(posedge clk) begin rec.b <= rec.b + 1; mem[0] <= din; end
endmodule
