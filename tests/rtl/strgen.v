// A string parameter's bytes on a port (tests/test_regress.py)
module strgen #(parameter STR = "none") (output [31:0] bytes);
assign bytes = STR;
endmodule
