#include <unistd.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

static int64_t ArithmeticRightShift(int64_t v, uint32_t amount) {
    if (v >= 0) {
        return v >> amount;
    } else {
        int64_t t = v;
        uint64_t u = *(uint64_t *)&t;
        u = 0x7FFFFFFFFFFFFFFFUL & u;
        u = u >> amount;
        uint64_t mask = (0xFFFFFFFFFFFFFFFFUL >> (63 - amount)) << (63 - amount);
        u = u | mask;
        return *(int64_t *)&u;
    }
}

static int64_t GetPositionRandom(int x, int y, int z) {
    long i = (long)(x * 3129871) ^ (long)z * 116129781L ^ (long)y;
    i = i * i * 42317861L + i * 11L;
    return ArithmeticRightShift(i, 16);
}

class Random {
public:
    explicit Random(int64_t seed)
        : seed(InitialScramble(seed))
    {
    }

    int nextInt(int n) {
        assert(n > 0);

        if ((n & -n) == n) {  // i.e., n is a power of 2
            return (int)((n * (long)next(31)) >> 31);
        }

        int bits, val;
        do {
            bits = next(31);
            val = bits % n;
        } while (bits - val + (n-1) < 0);
        return val;
    }

    int64_t nextLong() {
        return ((int64_t)next(32) << 32) + next(32);
    }

    int nextInt() {
        return next(32);
    }

private:
    int next(int bits) {
        int64_t oldSeed = seed;
        int64_t nextSeed = (oldSeed * multiplier + addend) & mask;
        seed = nextSeed;
        return (int)(nextSeed >> (48 - bits));
    }

    static int64_t InitialScramble(int64_t seed) {
        return (seed ^ multiplier) & mask;
    }

private:
    int64_t seed;

    static int64_t const multiplier = 0x5DEECE66DL;
    static int64_t const addend = 0xBL;
    static int64_t const mask = (1L << 48) - 1;
};

static int GetRandomItemIndex(int totalWeight, int weight) {
    for (int i = 0; i < totalWeight; i++) {
        int t = i;
        weight -= 1;
        if (weight < 0) {
            return t;
        }
    }
    return -1;
}

static void PrintHelpAndExit(string const& msg = "") {
    if (!msg.empty()) {
        cerr << "Error: " << msg << endl;
    }
    cerr << "dirt -f [facing:north,east,south,west] -r [rotation:comma separeted list of 0,1,2,3] -x [minX] -X [maxX] -y [minY] -Y [maxY] -z [minZ] -Z [maxZ]" << endl;
    cerr << "ROTATION" << endl;
    cerr << "    rotation = 0   rotation = 1   rotation = 2   rotation = 3" << endl;
    cerr << "    _____________  _____________  _____________  _____________" << endl;
    cerr << "    |         ==|  |           |  |           |  | I         |" << endl;
    cerr << "    |       ==  |  |           |  |           |  |  I        |" << endl;
    cerr << "    |           |  |           |  |           |  |           |" << endl;
    cerr << "    |           |  |        I  |  |  ==       |  |           |" << endl;
    cerr << "    |___________|  |_________I_|  |==_________|  |___________|" << endl;
    exit(1);
}

static vector<string> Split(string const&s, char delim) {
    vector<string> elems;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        if (!item.empty()) {
            elems.push_back(item);
        }
    }
    return elems;
}

enum Direction {
    DIRECTION_X = 1,
    DIRECTION_Y,
    DIRECTION_Z,
    DIRECTION_H,
};

enum Facing {
    FACING_NORTH = 1,
    FACING_EAST,
    FACING_SOUTH,
    FACING_WEST,
};

static int DirtRotation(int x, int y, int z) {
    int64_t pr = GetPositionRandom(x, y, z);
    Random rand(pr);
    int numFacingTypes = 4;
    int weight = abs((int)rand.nextLong()) % numFacingTypes;
    return GetRandomItemIndex(numFacingTypes, weight);
}

int main(int argc, char *argv[]) {
    int opt;
    int minX = INT_MAX;
    int maxX = INT_MIN;
    int minY = INT_MAX;
    int maxY = INT_MIN;
    int minZ = INT_MAX;
    int maxZ = INT_MIN;
    int facing = -1;
    int direction = DIRECTION_Y;
    vector<int> predicate;
    while ((opt = getopt(argc, argv, "d:r:f:x:X:y:Y:z:Z:")) != -1) {
        switch (opt) {
            case 'd': {
                string d = optarg;
                if (d == "y") {
                    direction = DIRECTION_Y;
                } else {
                    PrintHelpAndExit("unsupported direction");
                }
                break;
            }
            case 'f': {
                string f = optarg;
                if (f == "north") {
                    facing = FACING_NORTH;
                } else if (f == "east") {
                    facing = FACING_EAST;
                } else if (f == "south") {
                    facing = FACING_SOUTH;
                } else if (f == "west") {
                    facing = FACING_WEST;
                } else {
                    PrintHelpAndExit("unsupported facing");
                }
                break;
            }
            case 'r': {
                vector<string> tokens = Split(optarg, ',');
                for_each(tokens.begin(), tokens.end(), [&predicate](string const& s) {
                    int v = -1;
                    if (sscanf(s.c_str(), "%d", &v) != 1) {
                        PrintHelpAndExit("invlaid integer string");
                    }
                    predicate.push_back(v);
                });
                break;
            }
            case 'x':
                if (sscanf(optarg, "%d", &minX) != 1) {
                    PrintHelpAndExit();
                }
                break;
            case 'X':
                if (sscanf(optarg, "%d", &maxX) != 1) {
                    PrintHelpAndExit();
                }
                break;
            case 'y':
                if (sscanf(optarg, "%d", &minY) != 1) {
                    PrintHelpAndExit();
                }
                break;
            case 'Y':
                if (sscanf(optarg, "%d", &maxY) != 1) {
                    PrintHelpAndExit();
                }
                break;
            case 'z':
                if (sscanf(optarg, "%d", &minZ) != 1) {
                    PrintHelpAndExit();
                }
                break;
            case 'Z':
                if (sscanf(optarg, "%d", &maxZ) != 1) {
                    PrintHelpAndExit();
                }
                break;
        }
    }
    if (predicate.empty()) {
        PrintHelpAndExit("predicate is empty");
    }
    if (facing < 0) {
        PrintHelpAndExit("missing facing option (not supported yet)");
    }
    if (minX > maxX || minY > maxY || minZ > maxZ) {
        PrintHelpAndExit("invalid block range");
    }
    {
        int offset = 0;
        if (facing == FACING_EAST) {
            offset = 1;
        } else if (facing == FACING_SOUTH) {
            offset = 2;
        } else if (facing == FACING_WEST) {
            offset = 3;
        }
        for (int i = 0; i < predicate.size(); i++) {
            predicate[i] = (predicate[i] + offset) % 4;
        }
    }
    for (int y = minY; y <= maxY; y++) {
        for (int z = minZ; z <= maxZ; z++) {
            for (int x = minX; x <= maxX; x++) {
                bool ok = true;
                int bx = x;
                int by = y;
                int bz = z;
                for (int i = 0; i < predicate.size(); i++) {
                    int expected = predicate[i];
                    int actual = DirtRotation(bx, by, bz);
                    if (expected != actual) {
                        ok = false;
                        break;
                    }
                    if (direction == DIRECTION_X) {
                        bx++;
                    } else if (direction == DIRECTION_Y) {
                        by++;
                    } else if (direction == DIRECTION_Z) {
                        bz++;
                    }
                }
                if (ok) {
                    cout << "[" << x << ", " << y << ", " << z << "]" << endl;
                }
            }
        }
    }
}
