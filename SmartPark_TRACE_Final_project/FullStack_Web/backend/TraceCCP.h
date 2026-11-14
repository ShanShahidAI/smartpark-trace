#ifndef SMARTPARK_TRACE_CCP_H
#define SMARTPARK_TRACE_CCP_H
// Core CCP algorithms: only fixed raw arrays, pointers, singly-linked nodes,
// a hand-written FIFO queue, stack, AVL tree and sliding-window traversal.
// std::string is used only to store labels/plates; no STL DSA containers.
#include <string>
#include <ostream>
#include <istream>
#include <iomanip>
#include <cctype>
#include <algorithm>
namespace traceccp {
constexpr int MAX_VERTICES = 360;
constexpr int MAX_HOLDS = 64;
constexpr int MAX_GROUP_SIZE = 4;
struct Path {
    std::string at[MAX_VERTICES];
    int count = 0;
    bool found() const { return count > 0; }
    std::string text() const {
        std::string result;
        for (int i=0;i<count;++i) { if(i) result += " -> "; result += at[i]; }
        return result;
    }
};
class PathStack {
    struct Node {std::string value; Node* next; Node(const std::string& v,Node* n):value(v),next(n){} };
    Node* topNode=nullptr;
public:
    ~PathStack(){std::string ignored;while(pop(ignored)){} }
    void push(const std::string& s) { topNode=new Node(s,topNode); }
    bool pop(std::string& s) {
        if(!topNode)return false;
        Node* old=topNode; s=old->value;topNode=old->next;delete old;return true;
    }
};
// Matrix is a raw array. An edge may exist yet be temporarily blocked.
class RawGraph {
    std::string name[MAX_VERTICES];
    bool edge[MAX_VERTICES][MAX_VERTICES]{};
    bool blocked[MAX_VERTICES][MAX_VERTICES]{};
    int used=0;
    int find(const std::string& s) const {
        for(int i=0;i<used;++i)if(name[i]==s)return i;
        return -1;
    }
    int ensure(const std::string& s){
        int id=find(s); if(id>=0)return id;
        if(used>=MAX_VERTICES)return -1;
        name[used]=s;return used++;
    }
public:
    bool add(const std::string& a,const std::string& b,bool isClosed=false){
        int i=ensure(a),j=ensure(b);if(i<0||j<0||i==j)return false;
        edge[i][j]=edge[j][i]=true;
        blocked[i][j]=blocked[j][i]=isClosed;return true;
    }
    bool has(const std::string& a,const std::string& b) const {
        int i=find(a),j=find(b);return i>=0&&j>=0&&edge[i][j];
    }
    bool isClosed(const std::string& a,const std::string& b) const {
        int i=find(a),j=find(b);return i<0||j<0||!edge[i][j]||blocked[i][j];
    }
    Path bfs(const std::string& start,const std::string& destination) const {
        Path route;
        int src=find(start),dst=find(destination);
        if(src<0||dst<0)return route;
        bool seen[MAX_VERTICES]{};
        int parent[MAX_VERTICES],fifo[MAX_VERTICES],head=0,tail=0;
        for(int i=0;i<used;++i)parent[i]=-1;
        fifo[tail++]=src;seen[src]=true;
        while(head<tail){
            int curr=fifo[head++];if(curr==dst)break;
            for(int i=0;i<used;++i){
                if(edge[curr][i]&&!blocked[curr][i]&&!seen[i]){
                    seen[i]=true;parent[i]=curr;fifo[tail++]=i;
                }
            }
        }
        if(!seen[dst])return route;
        // Backtrack produces destination-first order. Linked stack reverses it.
        PathStack reverse;
        for(int n=dst;n!=-1;n=parent[n]) reverse.push(name[n]);
        std::string step;
        while(reverse.pop(step)&&route.count<MAX_VERTICES)route.at[route.count++]=step;
        return route;
    }
    Path nearestSafeExit(const std::string& start,char incidentBuilding,std::string& gate) const {
        Path best;
        gate.clear();
        for(char b='A';b<='C';++b) {
            if(b==incidentBuilding)continue; // affected building exit is treated as unsafe
            std::string exit=std::string("EXIT-")+b;
            Path candidate=bfs(start,exit);
            if(candidate.found()&&(!best.found()||candidate.count<best.count)){
                best=candidate;gate=exit;
            }
        }
        return best;
    }
};
class PlateAVL { // Raw pointer AVL by number plate, not numeric ticket.
    struct Node {
        std::string plate,slot;
        Node *left,*right;int height;
        Node(const std::string& p,const std::string& s):plate(p),slot(s),left(nullptr),right(nullptr),height(1){}
    };
    Node* root=nullptr;
    static int height(Node* n){return n?n->height:0;}
    static int max2(int a,int b){return a>b?a:b;}
    static void update(Node* n){n->height=1+max2(height(n->left),height(n->right));}
    static Node* right(Node* y){Node*x=y->left;Node*t=x->right;x->right=y;y->left=t;update(y);update(x);return x;}
    static Node* left(Node* x){Node*y=x->right;Node*t=y->left;y->left=x;x->right=t;update(x);update(y);return y;}
    static Node* put(Node* n,const std::string& p,const std::string& s){
        if(!n)return new Node(p,s);
        if(p<n->plate)n->left=put(n->left,p,s);
        else if(p>n->plate)n->right=put(n->right,p,s);
        else {n->slot=s;return n;}
        update(n);int b=height(n->left)-height(n->right);
        if(b>1&&p<n->left->plate)return right(n);
        if(b< -1&&p>n->right->plate)return left(n);
        if(b>1&&p>n->left->plate){n->left=left(n->left);return right(n);}
        if(b< -1&&p<n->right->plate){n->right=right(n->right);return left(n);}
        return n;
    }
    static void clear(Node*n){if(!n)return;clear(n->left);clear(n->right);delete n;}
public:
    PlateAVL()=default;
    PlateAVL(const PlateAVL&)=delete;
    PlateAVL&operator=(const PlateAVL&)=delete;
    ~PlateAVL(){clear(root);}
    void insert(const std::string& plate,const std::string& slot){root=put(root,plate,slot);}
    std::string findSlot(const std::string& plate,int& comparisons) const {
        comparisons=0;Node* n=root;
        while(n){++comparisons;if(n->plate==plate)return n->slot;n=plate<n->plate?n->left:n->right;}
        return "";
    }
};
struct Hold {
    std::string plate,type,owner,building,slot,path;
    int floor=0;
    bool active=false;
};
class RollbackStack {
    struct Node{Hold item;Node*next;Node(const Hold&h,Node*n):item(h),next(n){}};
    Node*top=nullptr;
public:
    ~RollbackStack(){Hold h;while(pop(h)){} }
    void push(const Hold&h){top=new Node(h,top);}
    bool pop(Hold&h){if(!top)return false;Node*n=top;h=n->item;top=n->next;delete n;return true;}
};
struct Group {
    std::string id,building;
    std::string plate[MAX_GROUP_SIZE],owner[MAX_GROUP_SIZE],type[MAX_GROUP_SIZE];
    int count=0,floor=0;bool vip=false;
};
class GroupQueue {
public:
    struct Node {Group group;Node*next;explicit Node(const Group&g):group(g),next(nullptr){}};
private:
    Node *head=nullptr,*tail=nullptr;int length=0;
public:
    GroupQueue()=default;
    GroupQueue(const GroupQueue&)=delete;
    GroupQueue&operator=(const GroupQueue&)=delete;
    ~GroupQueue(){clear();}
    const Node* first()const{return head;}
    int size() const{return length;}
    bool hasPlate(const std::string& plate)const{
        for(Node*n=head;n;n=n->next)for(int i=0;i<n->group.count;++i)
            if(n->group.plate[i]==plate)return true;
        return false;
    }
    bool hasId(const std::string& id)const{
        for(Node*n=head;n;n=n->next)if(n->group.id==id)return true;
        return false;
    }
    void push(const Group&g){Node*n=new Node(g);if(tail)tail->next=n;else head=n;tail=n;++length;}
    bool erase(const std::string& id){
        Node*prev=nullptr;
        for(Node*n=head;n;prev=n,n=n->next)if(n->group.id==id){
            if(prev)prev->next=n->next;else head=n->next;
            if(tail==n)tail=prev;
            delete n;--length;return true;
        }
        return false;
    }
    void clear(){while(head){Node*n=head;head=head->next;delete n;}tail=nullptr;length=0;}
};
class State {
public:
    bool evacuation=false;
    char incident='-';
    Hold holds[MAX_HOLDS];
    GroupQueue groups;
    RollbackStack undo;
    std::string lastRecovery;
    long long nextGroupId=1;
    State()=default;
    State(const State&)=delete;
    State&operator=(const State&)=delete;
    void clear(){evacuation=false;incident='-';nextGroupId=1;lastRecovery.clear();for(auto&h:holds)h=Hold();groups.clear();}
    bool heldSlot(const std::string&slot)const{
        for(const auto&h:holds)if(h.active&&h.slot==slot)return true;
        return false;
    }
    bool heldPlate(const std::string&plate)const{
        for(const auto&h:holds)if(h.active&&h.plate==plate)return true;
        return false;
    }
    int holdCount()const{int n=0;for(const auto&h:holds)if(h.active)++n;return n;}
    Hold* findHold(const std::string&plate){for(auto&h:holds)if(h.active&&h.plate==plate)return &h;return nullptr;}
    bool addHold(const Hold&h){
        if(heldPlate(h.plate)||heldSlot(h.slot))return false;
        for(auto&item:holds)if(!item.active){item=h;item.active=true;return true;}
        return false;
    }
    void removeHold(const std::string&plate){for(auto&h:holds)if(h.active&&h.plate==plate){h=Hold();return;}}
    bool pathUses(const std::string&serialized,const std::string&a,const std::string&b)const{
        if(serialized.empty())return false;
        std::string previous;int start=0;
        for(size_t i=0;i<=serialized.size();++i)if(i==serialized.size()||serialized[i]=='|'){
            std::string current=serialized.substr(start,i-start);
            if((previous==a&&current==b)||(previous==b&&current==a))return true;
            previous=current;start=(int)i+1;
        }
        return false;
    }
    static std::string packPath(const Path&p){
        std::string s;for(int i=0;i<p.count;++i){if(i)s+='|';s+=p.at[i];}return s;
    }
    bool write(std::ostream&out)const{
        out<<"CCP "<<(evacuation?1:0)<<' '<<incident<<' '<<nextGroupId<<' '<<std::quoted(lastRecovery)<<'\n';
        out<<"HOLDS "<<holdCount()<<'\n';
        for(const auto&h:holds)if(h.active)out<<std::quoted(h.plate)<<' '<<std::quoted(h.type)<<' '<<std::quoted(h.owner)
            <<' '<<std::quoted(h.building)<<' '<<h.floor<<' '<<std::quoted(h.slot)<<' '<<std::quoted(h.path)<<'\n';
        out<<"GROUPS "<<groups.size()<<'\n';
        for(auto*n=groups.first();n;n=n->next){
            const Group&g=n->group;
            out<<std::quoted(g.id)<<' '<<std::quoted(g.building)<<' '<<g.floor<<' '<<g.vip<<' '<<g.count;
            for(int i=0;i<g.count;++i)out<<' '<<std::quoted(g.plate[i])<<' '<<std::quoted(g.type[i])<<' '<<std::quoted(g.owner[i]);
            out<<'\n';
        }
        return out.good();
    }
};
} // traceccp
#endif
