#include <iostream>

using namespace std;

class addxy {
        int _x, _y;
public:
        void set_operands(int, int);
        int interpret();
};

void addxy::set_operands(int x, int y) {
        _x = x;
        _y = y;
}

int addxy::interpret() {
        _x = _x + _y;
        return _x;
}

int main() {
        addxy add;
        add.set_operands(1, 4);
        cout << "Result of in-place addition:" << add.interpret() << endl;
        cout << "Result of in-place addition:" << add.interpret() << endl;
        return 0;
}
