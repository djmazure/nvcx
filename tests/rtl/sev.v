// A report at each severity, all at the first clock edge (tests/test_nvc.py)
module sev (input clk, output reg q = 0);
always @(posedge clk) begin
    $info("sev note");
    $warning("sev warning");
    $error("sev error");
    $fatal(1, "sev failure");
    q <= 1;
end
endmodule
