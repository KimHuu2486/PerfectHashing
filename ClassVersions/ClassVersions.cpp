#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <chrono>

using namespace std;

struct Value {
    string key;
    string val;
};

class PerfectHashTable {
private:
    vector<vector<Value>> table;
    vector<int> setAHash1;
    vector<vector<int>> setAHash2;
    vector<bool> hasLevel2Hash;
    int m = 0;
    mt19937 gen;

    bool isSpaceOnly(string& s) {
        return all_of(s.begin(), s.end(), [](char ch) {
            return isspace(static_cast<unsigned char>(ch));
            });
    }

    bool isPrime(int n) {
        if (n < 2) return false;
        for (int i = 2; i * i <= n; i++) {
            if (n % i == 0) return false;
        }
        return true;
    }

    int findSmallestPrimeGreaterThan(int n) {
        while (!isPrime(n)) {
            n++;
        }
        return n;
    }

    int universalHash(const string& key, const vector<int>& setA, int mod) {
        long long sum = 0;
        int len = min(key.size(), setA.size());
        for (int i = 0; i < len; ++i) {
            int val = static_cast<int>(key[i]);
            if (val < 0 || val > 127) return -1;
            sum += val * setA[i];
        }
        return sum % mod;
    }

    void removeDuplicateKeys(vector<Value>& data) {
        unordered_set<string> seen;
        vector<Value> filtered;
        for (const auto& val : data) {
            if (seen.count(val.key)) continue;
            seen.insert(val.key);
            filtered.push_back(val);
        }
        data = move(filtered);
    }

public:
    PerfectHashTable() : gen(random_device{}()) {}

    void build(const string& path) {
        vector<Value> data;
        readFile(path, data);
        removeDuplicateKeys(data);
        int n = data.size();
        m = findSmallestPrimeGreaterThan(n);
        table.assign(m, vector<Value>());

        // Tạo ngẫu nhiên một tập hợp A1 với độ dài lớn hơn key lớn nhất (dự đoán <100)
        setAHash1.resize(100);
        uniform_int_distribution<> dis(0, m - 1);
        for (int& num : setAHash1) {
            num = dis(gen);
        }

        // Hash cấp 1
        for (const auto& cur : data) {
            int index = universalHash(cur.key, setAHash1, m);
            if (index < 0) continue;
            table[index].push_back(cur);
        }

        // Đặt cờ kiểm tra việc có xảy ra hàm băm cấp 2 hay không
        hasLevel2Hash.resize(m, false);
        // Hash cấp 2
        setAHash2.resize(m);

        for (int i = 0; i < m; ++i) {
            int sizeCollision = table[i].size();
            if (sizeCollision < 2) continue;

            hasLevel2Hash[i] = true;
            // Bảng băm cấp 2 với độ lớn bằng bình phương số lượng collision tại index thứ ith xảy ra ở hàm băm 1 để tỉ lệ xảy ra collision là tối ưu
            int sizeTable2 = sizeCollision * sizeCollision * 4;
            vector<pair<Value, bool>> res(sizeTable2);
            uniform_int_distribution<> dis2(0, sizeTable2 - 1);
            bool collision;
            // Lặp đến khi tạo ra được tập hợp A2 băm các collision tại index ith để không còn xảy ra collision ở hàm băm 2
            do {
                collision = false;
                fill(res.begin(), res.end(), make_pair(Value{ "", "" }, false));

                // Tạo ngẫu nhiên tập hợp A2
                setAHash2[i].clear();
                setAHash2[i].resize(100);
                for (int& num : setAHash2[i]) {
                    num = dis2(gen);
                }

                for (const auto& s : table[i]) {
                    int index2 = universalHash(s.key, setAHash2[i], sizeTable2);
                    if (res[index2].second) {
                        collision = true;
                        break;
                    }
                    res[index2] = { s, true };
                }
            } while (collision);

            table[i].clear();
            table[i].resize(sizeTable2);
            for (int j = 0; j < sizeTable2; j++) {
                table[i][j] = res[j].first;
            }
        }
    }

    void readFile(const string& path, vector<Value>& vec) {
        ifstream ifs(path);
        if (!ifs.is_open()) {
            cerr << "Can't open file: " << path << endl;
            return;
        }
        string temp;
        Value cur;
        while (getline(ifs, temp)) {
            if (temp.empty() || isSpaceOnly(temp) || temp.size() < 3) continue;
            stringstream ss(temp);
            ss >> cur.key;
            cur.val = temp;
            vec.push_back(cur);
        }
        ifs.close();
    }

    string search(const string& key) {
        int index1 = universalHash(key, setAHash1, table.size());

        if (index1 >= table.size() || table[index1].empty()) {
            return "Not found";
        }

        if (!hasLevel2Hash[index1]) {
            // Không có băm cấp 2, kiểm tra key
            if (table[index1].begin()->key == key) {
                return table[index1].begin()->val;
            }
            return "Not found";
        }

        // Có dùng băm cấp 2
        int index2 = universalHash(key, setAHash2[index1], table[index1].size());
        if (index2 >= table[index1].size() || table[index1][index2].key != key) {
            return "Not found";
        }

        return table[index1][index2].val;
    }

};

int main() {
    string path;
    cout << "Input the path of file: ";
    cin >> path;

    PerfectHashTable pht;
    auto start = chrono::high_resolution_clock::now();
    pht.build(path);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Time for hashing data: " << duration.count() << "ms" << endl << endl;

    int n;
    cout << "Input the numbers of words which you want to search: ";
    cin >> n;
    vector<string> vec(n);
    cout << "Input " << n << " words: " << endl;
    for (string& s : vec) {
        cin >> s;
    }

    const int repeat = 1000000; // lặp lại 1 triệu lần

    for (const string& s : vec) {
        start = chrono::high_resolution_clock::now();
        string temp;
        for (int i = 0; i < repeat; ++i) {
            temp = pht.search(s);
        }
        end = chrono::high_resolution_clock::now();

        auto duration_ns = chrono::duration_cast<chrono::nanoseconds>(end - start);
        long long duration_ps = duration_ns.count() * 1000; // nanoseconds -> picoseconds
        cout << "Result: " << pht.search(s) << endl;
        cout << "Average time search (per operation): " << duration_ps / repeat << " ps" << endl;
    }

    return 0;
}
