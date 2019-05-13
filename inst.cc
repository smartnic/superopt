#include <iostream>

#define NUM_REGS 4

using namespace std;

class prog_state {
        int pc = 0; /* Assume only straight line code execution for now */
public:
        int regs[NUM_REGS] = {}; /* assume only registers for now */
        void print();
};

void prog_state::print() {
        for (int i = 0; i < NUM_REGS; i++) {
                cout << "Register "  << i << " " << regs[i] << endl;
        }
}

class inst {
 public:
  inst();
  virtual int interpret(prog_state &ps);
};

class addxy {
        int _x, _y;
public:
        addxy(int x, int y) { _x = x; _y = y; }
        int interpret(prog_state &ps);
};

class movxc {
        int _x, _c;
public:
        movxc(int x, int c) { _x = x; _c = c; }
        int interpret(prog_state &ps);
};

int addxy::interpret(prog_state &ps) {
        ps.regs[_x] = ps.regs[_x] + ps.regs[_y];
        return ps.regs[_x];
}

int movxc::interpret(prog_state &ps) {
        ps.regs[_x] = _c;
        return ps.regs[_x];
}

int main() {
        movxc mov1(1, 10);
        movxc mov2(2, 4);
        addxy add(1, 2);
        
        prog_state ps;

        cout << "Result of first move:" << mov1.interpret(ps) << endl;
        ps.print();
        cout << "Result of second move:" << mov2.interpret(ps) << endl;
        ps.print();
        cout << "Result of in-place addition:" << add.interpret(ps) << endl;
        ps.print();
        return 0;
}
