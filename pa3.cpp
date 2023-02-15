#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <time.h>

using namespace std;

class Node;

class Net {
    public:
        string name;
        vector<Node *> nodes;
        int numberOfNodesInPartA;
        int numberOfNodesInPartB;
        Net(string &name) : name(name) {
        }
        void print();
};


class Node {
    public:
        Node(string name) {
            this->name = name;
            this->isLocked = false;
            this->next = NULL;
            this->previous = NULL;
        }
    // 一開始input就決定好的
        string name;
        vector<Net *> nets;
    // 計算的時候，會需要設定＆修改的欄位
        int gain;
        bool isInPartA;
        bool isLocked;
    // double linked list 需要
        Node *next;
        Node *previous;
};

void Net::print() {
    cout << "NET " << this->name << " { ";
    for (int i=0; i<this->nodes.size(); i++) {
        cout << this->nodes[i]->name << " ";
        if (i == this->nodes.size() - 1) cout << "}"<< endl;
    }
}

// pmax
// = max {pi} = max { ci連到幾個net | 所有node ci }
int maxDegree(vector<Node *> &nodes) {
    int max = 0;
    for (int i=0; i<nodes.size(); i++) {
        int degree = nodes[i]->nets.size();
        if (degree > max) max = degree;
    }
    return max;
}

class SortedBucketList {
    public:
    // 要知道pmax，才知道vector要開幾格
        int pmax;
    // 真正在負責存東西的vector
        vector<Node *> list;
    // 在新增或是刪掉東西的時候，同時要確保_size和maxGain都是正確的
        int _size;
        int maxGain;
        SortedBucketList(int pmax) {
            this->pmax = pmax;
            this->maxGain = -pmax;
            for (int i=0; i <= pmax * 2; i++) {
                this->list.push_back(NULL);
            }
        }
        int size() {
            return this->_size;
        }
        int indexOf(int gain) {
            return (gain - (-pmax));
        }
        int gainOf(int index) {
            return (index - pmax);
        }
        // 把一個node，根據gain值，放到這個list中
        void push_node(Node *node, int gain) {
            int index = indexOf(gain);
            // 如果這個位置沒有東西，代表這個node是第一個
            if (this->list[index] == NULL) {
                this->list[index] = node;
                if (gain > this->maxGain) {
                    this->maxGain = gain;
                };
            } else {
                node->next = this->list[index];
                this->list[index]->previous = node;
                this->list[index] = node;
            }
            this->_size++;
        }
        // 把一個node，從這個list中gain對應的位置上刪掉
        void delete_node(Node *node, int gain) {
            int index = indexOf(gain);
            if (this->list[index] == node) {
                this->list[index] = node->next;
                if (node->next) node->next->previous = NULL;
                if ((gain == this->maxGain) && (node->next == NULL)) {
                    this->reset_max();
                }
            } else {
                if (node->previous) node->previous->next = node->next;
                if (node->next) node->next->previous = node->previous;
            }
            node->next = NULL;
            node->previous = NULL;
            this->_size--;
        }
        void reset_max() {
            this->maxGain = -(this->pmax);
            for (int i = this->list.size() - 1; i >= 0; i--) {
                if (this->list[i] != NULL) {
                    this->maxGain = this->gainOf(i);
                    return;
                }
            }
        }

        void changeGain(Node *node, int oldGain, int newGain) {
            this->delete_node(node, oldGain);
            this->push_node(node, newGain);
        }
        Node *pop_max() {
            int index = indexOf(this->maxGain);
            Node *maxGainNode = this->list[index];
            if (maxGainNode != NULL) this->delete_node(maxGainNode, this->maxGain);
            return maxGainNode;
        }
};

// 如果在nodes裡面，就回答它的位置，不然就回答-1
int findIndex(vector<Node *> &nodes, string nodeName) {
    for (int i=0; i<nodes.size(); i++) {
        if (nodes[i]->name == nodeName) return i;
    }
    return -1;
}

class Solver {
    public:
        vector<Node *> nodes;
        vector<Net *> nets;
        SortedBucketList *listA;
        SortedBucketList *listB;
        string inputFile;
        Solver(string inputFile) : inputFile(inputFile) {}
        // 計算當前的partition中有幾個node在part A裡面
        int aCount() {
            int count = 0;
            for (int i=0; i<this->nodes.size(); i++) {
                if (this->nodes[i]->isInPartA) count++;
            }
            return count;
        }
        // 計算當前的partition中有幾個node在part B裡面
        int bCount() {
            return (this->nodes.size() - this->aCount());
        }
        // 給兩個A, B的值，判斷要不要走
        bool shouldMove(int sizeA, int sizeB) {
            int n = this->nodes.size();
            return ((sizeA <= 0) || (sizeA >= n));
        }
        // 給兩個A, B的值，判斷是否平衡
        bool isBalanced(int sizeA, int sizeB) {
            int n = this->nodes.size();
            return (((n - 2) < (2 * sizeA)) && ((2 * sizeA) < (n + 2)));
        }
        // 判斷當前partition的A,B是否平衡
        bool isBalanced() {
            return this->isBalanced(this->aCount(), this->bCount());
        }
        // 計算當前的partition有幾個nets被切到
        int cutSize() {
            int cutCount = 0;
            for (int i=0; i<this->nets.size(); i++) {
                int aCount = 0;
                int bCount = 0;
                for (int j=0; j<this->nets[i]->nodes.size(); j++) {
                    if (this->nets[i]->nodes[j]->isInPartA) aCount++;
                    else bCount++;
                    if ((aCount > 0) && (bCount > 0)) {
                        cutCount++;
                        break;
                    }
                }
            }
            return cutCount;
        }

        // 把當前的partition存成作業的格式的string
        string getOutputString() {
            int cutCount = this->cutSize();
            string output = "";
            output += "cut_size " + to_string(cutCount) + "\n";
            output += "A\n";
            for (int i=0; i<this->nodes.size(); i++) {
                if (this->nodes[i]->isInPartA == true) output += this->nodes[i]->name + "\n";
            }
            output += "B\n";
            for (int i=0; i<this->nodes.size(); i++) {
                if (this->nodes[i]->isInPartA == false) output += this->nodes[i]->name + "\n";
            }
            return output;
        }
        // 把當前的partition印出來
        void printPartition() {
            int cutCount = this->cutSize();
            cout << "cut_size " << cutCount << endl;
            cout << "A: ";
            for (int i=0; i<this->nodes.size(); i++) {
                if (this->nodes[i]->isInPartA == true) cout << this->nodes[i]->name << " ";
            }
            cout << endl;
            cout << "B: ";
            for (int i=0; i<this->nodes.size(); i++) {
                if (this->nodes[i]->isInPartA == false) cout << this->nodes[i]->name << " ";
            }
            cout << endl;
        }

        // 把node根據新的gain值，移到所屬的list的對應的位置上
        void changeGain(Node *node, int oldGain, int newGain) {
            node->gain = newGain;
            if (node->isInPartA) this->listA->changeGain(node, oldGain, newGain);
            else this->listB->changeGain(node, oldGain, newGain);
        }

        string solve() {
            // 讀資料
            ifstream inFile(this->inputFile, ios::in);
            string line;
            while (true) {
                // 讀一行，存到line
                getline(inFile, line);
                if (line == "") break;
                // 把line根據空白字元切成很多個string，放在ss
                stringstream ss(line);
                // 從ss讀一個string，存到NET
                string NET;
                ss >> NET;
                // 從ss讀一個string，存到netName
                string netName;
                ss >> netName;
                // 用netName創一個Net物件
                Net *newNet = new Net(netName);
                Node *node;
                while (true) {
                    string temp;
                    ss >> temp;
                    // 如果是開始的符號，就不管它
                    if (temp == "{") continue;
                    // 如果是結束的符號，就離開迴圈
                    if (temp == "}") break;
                    // 不然temp就是node的名稱
                    string nodeName = temp;
                    int index = findIndex(this->nodes, nodeName);
                    if (index == -1) {
                        node = new Node(nodeName);
                        this->nodes.push_back(node);
                        index = this->nodes.size() - 1;
                    }
                    newNet->nodes.push_back(this->nodes[index]);    // 把這個node登記給net
                    this->nodes[index]->nets.push_back(newNet);    // 把這個net登記給node
                }
                this->nets.push_back(newNet);
            }
            // 初始化random seed
            srand(time(NULL));
            // 跑30次選cut size最小的，存到output
            int minCutSize = 2147483647;
            string output;
            for (int trialCount=0; trialCount<100000; trialCount++) {
                // 隨機指定nodes的分組
                for (int i=0; i<this->nodes.size(); i++) {
                    // 根據一個隨機值的奇偶，來決定放到A還是B
                    if (rand() % 2) this->nodes[i]->isInPartA = true;
                    else this->nodes[i]->isInPartA = false;
                }
                // 為了要決定SortedBucketList要多大，先把pmax算好
                int pmax = maxDegree(nodes);
                // 跑很多個pass，直到沒辦法再更好，算出這一大輪的結果
                while (true) {
                    // 幫A, B都個創一個空的sorted bucket list
                    this->listA = new SortedBucketList(pmax);
                    this->listB = new SortedBucketList(pmax);
                    // 創一個list來存「被locked住的nodes」
                    vector<Node *> lockedList;  // 其實就是這輪pass的g1-gn
                    // 把所有node的gain都算出來，綁到對應的sorted bucket list對應的位置上
                    //    (1) 先算好每個net n的A(n)和B(n)（分別有幾個node在part A和part B）
                    for (int i=0; i<this->nets.size(); i++) {
                        Net *net = this->nets[i];
                        int countA = 0;
                        int countB = 0;
                        for (int j=0; j<net->nodes.size(); j++) {
                            if (net->nodes[j]->isInPartA) countA++;
                            else countB++;
                        }
                        net->numberOfNodesInPartA = countA; // A(n): 這個net有幾個nodes在part A裡面
                        net->numberOfNodesInPartB = countB; // B(n): 這個net有幾個nodes在part B裡面
                    }
                    // cout << "=== gain ===\n";
                    //    (2) 在根據算好的A(n)和B(n)，用論文裡"compute cell gains"的演算法算出所有nodes的gain值
                    for (int i=0; i<this->nodes.size(); i++) {
                        Node *node = nodes[i];
                        int gain = 0;
                        for (int j=0; j<node->nets.size(); j++) {
                            Net *net = node->nets[j];
                            int Fn; // F(n)
                            int Tn;  // T(n)
                            if (node->isInPartA) {              // 如果這個node在A這邊
                                Fn = net->numberOfNodesInPartA; // 那 F = from-block = 移動的來源 = A，所以 F(n) = A(n)
                                Tn = net->numberOfNodesInPartB;  // 那 T = to-block = 移動的目的地 = B，所以 T(n) = B(n)
                            } else {                            // 同理：
                                Fn = net->numberOfNodesInPartB; // F(n) = B(n)
                                Tn = net->numberOfNodesInPartA;  // T(n) = A(n)
                            }
                            if (Fn == 1) gain++; // IF F(n) = 1 THEN increment g(i)
                            if (Tn == 0) gain--;  // IF T(n) = 0 THEN increment g(i)
                        }
                        // 把gain值存到node裡面備用
                        node->gain = gain;
                        // cout << node->name << ": " << gain << endl;
                        // 把node根據算好的gain值，掛到對應個list上
                        if (node->isInPartA) listA->push_node(node, gain);  // 這個node在A裡面，就掛到A的sorted bucket list
                        else listB->push_node(node, gain);  // 不然就掛到B的list
                    }
                    // 等等的for裡面，要用兩個變數紀錄part A和part B大小的變動，以此來判斷平衡性，決定要移動哪個node
                    // 如果移動某個node，會讓sizeA和sizeB差太多的話，就不要移動
                    int sizeA = this->aCount();
                    int sizeB = this->bCount();
                    // cout << "==============\n";
                    // 在試移動g1...gn的每一步的時候，也紀錄一下哪個k能讓g1+...+gk最大
                    int gainSum = 0;                // 計算到這一步為止的累加值
                    int maxSum = -2147483648;       // 存目前的最大的g1+...+gk
                    int k = 0;                      // 存目前讓g1+...+gk最大的k
                    for (int lockCount=0; lockCount<this->nodes.size(); lockCount++) {
                        // 把gain最大的node找出來，從對應個sorted bucket list裡面拿出來，放到lockedList裡面
                        Node *maxGainNode;
                        int gain;
                        // cout << "====\n";
                        // cout << "A " << sizeA << "; B " << sizeB << endl;
                        // cout << "maxA " << this->listA->maxGain << "; maxB " << this->listB->maxGain << endl;
                        if (((this->listA->maxGain > this->listB->maxGain)   // 「如果最大值現在在A裡面
                                && (shouldMove(sizeA-1, sizeB+1)))           //  ，而且把它從A拿到B並不會破壞平衡」
                            || (shouldMove(sizeA+1, sizeB-1))) {            // 或是「移B會失衡」
                            gain = this->listA->maxGain;
                            maxGainNode = this->listA->pop_max();
                            sizeA--;
                            sizeB++;
                        } else if (shouldMove(sizeA+1, sizeB-1)) { // 「如果最大值現在在B裡面， 或是『A已經太大，再移動會失衡』」而且「從B拿東西並不會導致失衡」
                            gain = this->listB->maxGain;
                            maxGainNode = this->listB->pop_max();
                            sizeB--;
                            sizeA++;
                        } else {    // 怎麼移都會失衡的情況
                            // cout << "break" << endl;
                            // cout << this->nodes.size() << endl;
                            // cout << isBalanced(sizeA-1, sizeB+1) << endl;
                            break;
                        }
                        // 如果「拿了不會失衡的那一方，其實也沒有東西可以拿了（有可能已經全部都lock住了）」，就提早結束
                        if (maxGainNode == NULL) break;
                        // cout << "move " << maxGainNode->name << "; gain " << gain << "; sum " << gainSum + gain << endl;
                        
                        // 不然就把這個node加到locked list（存g1-gn的list）
                        lockedList.push_back(maxGainNode);
                        maxGainNode->isLocked = true;

                        // 更新maxGainNode的所有neighbor（照論文的方法算）
                        for (int i=0; i<maxGainNode->nets.size(); i++) {
                            Net *net = maxGainNode->nets[i];
                            // (0) 先把F(n)和T(n)算好備用
                            int Fn; // F
                            int Tn; // T
                            if (maxGainNode->isInPartA) {
                                Fn = net->numberOfNodesInPartA; // F = A
                                Tn = net->numberOfNodesInPartB;  // T = B
                            } else {
                                Fn = net->numberOfNodesInPartB; // F = B
                                Tn = net->numberOfNodesInPartA;  // T = A
                            }
                            // cout << "net " << net->name << "(" << Fn << "," << Tn << ")"<< endl;
                            /* check critical nets before the move */
                            if (Tn == 0) { // (1)
                                // 把所有unlocked nodes的gain都加一
                                for (int j=0; j<net->nodes.size(); j++) {
                                    Node *node = net->nodes[j];
                                    if (node->isLocked == false) {
                                        this->changeGain(node, node->gain, node->gain + 1);
                                    }
                                }
                            } else if (Tn == 1) { // (2)
                                // 把唯一在T裡面的那個node的gain減一（如果是unlocked）
                                for (int j=0; j<net->nodes.size(); j++) {
                                    Node *node = net->nodes[j];
                                    if (node->isInPartA != maxGainNode->isInPartA) { // 在T裡面 = 跟當前要移的maxGainNode不同方向
                                        if (node->isLocked == false) this->changeGain(node, node->gain, node->gain - 1);
                                        break;
                                    }
                                }
                            }
                            // do the move: decrement F(n) and increment T(n);
                            Fn--; // (3)-1
                            Tn++; // (4)-1
                            if (maxGainNode->isInPartA) {
                                net->numberOfNodesInPartA--; // (3)-2 （）
                                net->numberOfNodesInPartB++; // (4)-2
                            } else {
                                net->numberOfNodesInPartA++; // (4)-2
                                net->numberOfNodesInPartB--; // (3)-2
                            }
                            // check critical nets after the move
                            if (Fn == 0) { // (5)
                                // 把所有unlocked nodes的gain都「減一」
                                for (int j=0; j<net->nodes.size(); j++) {
                                    Node *node = net->nodes[j];
                                    if (node->isLocked == false) {
                                        this->changeGain(node, node->gain, node->gain - 1);
                                    }
                                }
                            } else  if (Fn == 1) {  // (6)
                                // 把唯一在F裡面的那個node的gain「加一」（如果是unlocked）
                                for (int j=0; j<net->nodes.size(); j++) {
                                    Node *node = net->nodes[j];
                                    if (node->isInPartA == maxGainNode->isInPartA) { // 在F裡面 = 跟當前要移的maxGainNode同一個方向
                                        if (node->isLocked == false) this->changeGain(node, node->gain, node->gain + 1);
                                        break;
                                    }
                                }

                            }

                        }
                        // 更新目前「g1+...+gk的最大值」和「k」
                        gainSum += gain;
                        if (gainSum > maxSum) {
                            maxSum = gainSum;
                            k = lockCount + 1;
                        }

                        // // 印出所有nodes的gain
                        // cout << "=== gain ===\n";
                        // for (int i=0; i<this->nodes.size(); i++) {
                        //     Node *node = nodes[i];
                        //     cout << node->name << ": " << node->gain << endl;
                        // }
                    }

                    // cout << "--> max sum: " << maxSum << endl;

                    // 如果改了也沒幫助，或是根本沒有要改，就是算完了
                    if ((maxSum <= 0) || (k == 0)) break;

                    // 真正去改每個node裡面，設定的分組
                    for (int i=0; i<k; i++) {
                        // 把分組改掉
                        lockedList[i]->isInPartA = !(lockedList[i]->isInPartA);
                        // cout << "--> move " << lockedList[i]->name << endl;
                        // this->printPartition();
                    }
                    // 把全部的lock解開，準備下一輪
                    for (int i=0; i<nodes.size(); i++) {
                        nodes[i]->isLocked = false;
                    }
                }
                // 把答案印出來
                // cout << "===========================\n";
                // this->printPartition();

                // 如果這一大輪的答案更好，就把output string存起來
                if (this->isBalanced()) {
                    if (this->cutSize() < minCutSize) {
                        minCutSize = this->cutSize();
                        // cout << minCutSize << endl;
                        output = this->getOutputString();
                    }
                }

                // 重新把nodes全部初始化
                for (int i=0; i<nodes.size(); i++) {
                    // 從locked list上拿掉
                    nodes[i]->isLocked = false;
                    // 從兩個sorted bucket list拿掉
                    nodes[i]->next = NULL;
                    nodes[i]->previous = NULL;
                }
            }
            return output;
        }
};

int main(int argc, char *argv[]) {
    string inputFile(argv[1]);
    ofstream outfile(argv[2], ios::out);
    outfile << (new Solver(inputFile))->solve();
    return 0;
}
