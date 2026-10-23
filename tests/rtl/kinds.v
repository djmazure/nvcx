`timescale 1ns/1ps
package kinds_pkg; int pkg_x = 3; endpackage
module sub(input logic a, output logic y); assign y = ~a; endmodule
module kinds(input logic clk, output logic q);
  logic s = 0; int cnt = 0; wire w = s; localparam P = 4; parameter Q = 5;
  logic [3:0] arr [0:1];
  genvar i;
  generate for (i = 0; i < 2; i++) begin : gen_blk
    logic g = 0;
    sub u(.a(g), .y());
  end endgenerate
  generate if (P > 1) begin : ifg
    logic ig = 1;
  end endgenerate
  function automatic int f(int x); return x + 1; endfunction
  task automatic t(); cnt <= cnt + 1; endtask
  always @(posedge clk) begin s <= ~s; cnt <= f(cnt); end
  initial begin #1 cnt = 0; end
  always_comb q = s;
endmodule
