#include <unistd.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include <sstream>
#include <thread>
#include <set>
#include <list>
#include "hwm/task/task_queue.hpp"

using namespace std;

constexpr bool IsSignedRightShiftArithmetic() {
    return -1 >> 1 == -1;
}

static int64_t ArithmeticRightShift(int64_t v, uint32_t amount) {
    if constexpr (IsSignedRightShiftArithmetic()) {
        return v >> amount;
    } else {
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

class strings {
    strings() = delete;

public:
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

    static bool StartsWith(string const &s, string const &search) {
        if (s.size() < search.size()) {
            return false;
        }
        if (search.empty()) {
            return false;
        }
        auto idx = s.find(search);
        if (idx == string::npos) {
            return false;
        }
        return idx == 0;
    }
    
    static bool EndsWith(string const &s, string const &search) {
        if (s.size() < search.size()) {
            return false;
        }
        if (search.empty()) {
            return false;
        }
        auto idx = s.rfind(search);
        if (idx == string::npos) {
            return false;
        }
        return idx + search.size() == s.size();
    }
    
    static string LTrim(string const &s, string const &left) {
        if (left.empty()) {
            return s;
        }
        string ret = s;
        while (StartsWith(ret, left)) {
            ret = ret.substr(left.size());
        }
        return ret;
    }
    
    static string RTrim(string const &s, string const &right) {
        if (right.empty()) {
            return s;
        }
        string ret = s;
        while (EndsWith(ret, right)) {
            ret = ret.substr(0, ret.size() - right.size());
        }
        return ret;
    }
    
    static string Trim(string const& s, string delim = " ") {
        return LTrim(RTrim(s, delim), delim);
    }
    
    static int ToInt(string const& s) {
        int v = -1;
        if (sscanf(s.c_str(), "%d", &v) != 1) {
            throw runtime_error("invlaid integer string: " + s);
        }
        return v;
    }
};

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

struct Predicate {
    int dx;
    int dy;
    int dz;
    int rotation;

    /**
        {
            dx: 0,
            dy: 0,
            dz: 0,
            r: 0
        }
     */
    static Predicate FromJSON(string const& s) {
        string t = strings::LTrim(strings::RTrim(s, " "), " ");
        if (!strings::StartsWith(t, "{")) {
            throw runtime_error("invalid json: " + s);
        }
        if (!strings::EndsWith(t, "}")) {
            throw runtime_error("invalid json: " + s);
        }
        vector<string> tokens = strings::Split(t.substr(1, t.size() - 2), ',');
        optional<int> dx;
        optional<int> dy;
        optional<int> dz;
        optional<int> r;
        for (auto const& token : tokens) {
            vector<string> pp = strings::Split(strings::Trim(token), ':');
            if (pp.size() != 2) {
                throw runtime_error("invalid json: " + token);
            }
            auto key = strings::Trim(pp[0]);
            auto value = strings::Trim(pp[1]);
            if (key == "dx") {
                dx = strings::ToInt(value);
            } else if (key == "dy") {
                dy = strings::ToInt(value);
            } else if (key == "dz") {
                dz = strings::ToInt(value);
            } else if (key == "r") {
                r = strings::ToInt(value);
            } else {
                throw runtime_error("unknown key for Predicate: key=" + key);
            }
        }
        if (!dx || !dy || !dz || !r) {
            throw runtime_error("incomplete json value: " + s);
        }
        Predicate p;
        p.dx = *dx;
        p.dy = *dy;
        p.dz = *dz;
        p.rotation = *r;
        return p;
    }
};

static bool SatisfiesPredicates(int x, int y, int z, Predicate const* predicate, size_t predicateSize) {
    for (int i = 0; i < predicateSize; i++) {
        Predicate p = predicate[i];
        int bx = x + p.dx;
        int by = y + p.dy;
        int bz = z + p.dz;
        int actual = DirtRotation(bx, by, bz);
        if (p.rotation != actual) {
            return false;
        }
    }
    return true;
}

static mutex coutMutex;
using u128 = __uint128_t;

template<class T>
static T IndexFromCoord3(T a, T b, T c, T minA, T maxA, T minB, T maxB, T minC, T maxC) {
    T const dA = maxA - minA + 1;
    T const dB = maxB - minB + 1;
    T const dC = maxC - minC + 1;
    return a * dB * dC + b * dC + c;
}

template<class T>
static void CoordFromIndex3(T idx, T *a, T *b, T *c, T minA, T maxA, T minB, T maxB, T minC, T maxC) {
    T const dB = maxB - minB + 1;
    T const dC = maxC - minC + 1;
    T const r = idx % (dB * dC);
    T const ta = (idx - r) / (dB * dC);
    T const tc = r % dC;
    T const tb = (r - tc) / dC;
    *a = ta + minA;
    *b = tb + minB;
    *c = tc + minC;
}

static void ExecuteTask(int direction, u128 begin, u128 end, Predicate const* predicates, size_t numPredicates, int minX, int maxX, int minY, int maxY, int minZ, int maxZ) {
    u128 idx = begin;
    u128 x0, y0, z0;
    CoordFromIndex3<u128>(idx, &y0, &z0, &x0, minY, maxY, minZ, maxZ, minX, maxX);
    for (int y = y0; y <= maxY && idx < end ; y++) {
        for (int z = z0; z <= maxZ && idx < end; z++) {
            for (int x = x0; x <= maxX && idx < end; x++, idx++) {
                if (SatisfiesPredicates(x, y, z, predicates, numPredicates)) {
                    lock_guard<mutex> l(coutMutex);
                    cout << "[" << x << ", " << y << ", " << z << "]" << endl;
                }
            }
            x0 = minX;
        }
        z0 = minZ;
    }
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
    vector<int> simplePredicate;
    vector<Predicate> predicate;
    while ((opt = getopt(argc, argv, "d:r:f:x:X:y:Y:z:Z:p:")) != -1) {
        switch (opt) {
            case 'd': {
                string d = optarg;
                if (d == "y") {
                    direction = DIRECTION_Y;
                } else if (d == "x") {
                    direction = DIRECTION_X;
                } else if (d == "z") {
                    direction = DIRECTION_Z;
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
                vector<string> tokens = strings::Split(optarg, ',');
                for_each(tokens.begin(), tokens.end(), [&simplePredicate](string const& s) {
                    int v = -1;
                    if (sscanf(s.c_str(), "%d", &v) != 1) {
                        PrintHelpAndExit("invlaid integer string");
                    }
                    simplePredicate.push_back(v);
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
            case 'p': {
                vector<string> tokens = strings::Split(optarg, '}');
                for_each(tokens.begin(), tokens.end(), [&predicate](string const& s) {
                    Predicate p = Predicate::FromJSON(strings::LTrim(s + "}", ","));
                    predicate.push_back(p);
                });
                break;
            }
        }
    }
    
    if (!simplePredicate.empty()) {
        if (!predicate.empty()) {
            throw runtime_error("both of -r and -p option are set");
        }
        int dx = 0;
        int dy = 0;
        int dz = 0;
        switch (direction) {
            case DIRECTION_X:
                dx = 1;
                break;
            case DIRECTION_Y:
                dy = 1;
                break;
            case DIRECTION_Z:
                dz = 1;
                break;
        }
        int x = 0;
        int y = 0;
        int z = 0;
        for (int i = 0; i < simplePredicate.size(); i++) {
            int r = simplePredicate[i];
            Predicate p;
            p.dx = x;
            p.dy = y;
            p.dz = z;
            p.rotation = r;
            predicate.push_back(p);
            x += dx;
            y += dy;
            z += dz;
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
            predicate[i].rotation = (predicate[i].rotation + offset) % 4;
        }
    }
    u128 const volume = u128(maxX - minX + 1) * u128(maxY - minY + 1) * u128(maxZ - minZ + 1);
    unsigned int const concurrency = thread::hardware_concurrency();
    size_t volumePerTask = volume / concurrency;
    list<future<void>> futures;
    hwm::task_queue tq(concurrency);
    for (unsigned int i = 0; i < concurrency; i++) {
        u128 const begin = i * volumePerTask;
        u128 const end = i == concurrency - 1 ? volume : begin + volumePerTask;
        futures.emplace_back(tq.enqueue(ExecuteTask, direction, begin, end, predicate.data(), predicate.size(), minX, maxX, minY, maxY, minZ, maxZ));
    }
    for (auto& f : futures) {
        f.get();
    }
}
