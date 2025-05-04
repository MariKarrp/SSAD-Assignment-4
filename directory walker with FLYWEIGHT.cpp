//Warning: the code was written using solutions and pattern applications from Lab Solutions (week 12 FLYWEIGHT, week 13 ITERATOR & VISITOR)
#include <iostream>
#include <vector>
#include <unordered_map> //was used following the solution from the Lab Solutions week 12
#include <memory>
#include <iomanip> //for setprecision() call
#include <algorithm>
#include <windows.h>
#include <psapi.h>
//CHECKER MEMORY USAGE
void printMemoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(),(PROCESS_MEMORY_COUNTERS*)&pmc,sizeof(pmc))) {
        std::cout << "Used: " << pmc.PrivateUsage / 1024 << " KB\n";
    }
}
using namespace std;
//to control numbers, for example 12.0 -> 12, 4668366.8000000000003 -> 4668366.8 (maybe it's an extra stuff, but i thought that task "size is a floating-point number" means 1 number after dot)
static string afterDOT(double size) {
    ostringstream output;
    output << fixed << setprecision(1);
    output << size;
    string result = output.str();
    if (result.find(".0") == result.size() - 2) {
        result.erase(result.size() - 2);
    }
    return result;
}
//Shared properties (FLYWEIGHT)
class FileProperties {
private:
    string extension;
    bool readOnly;
    string owner;
    string group;
public:
    FileProperties(string extension, bool readOnly, string owner, string group) {
        this -> extension = extension;
        this -> readOnly = readOnly;
        this -> owner = owner;
        this -> group = group;
    }
    string& getExtension() {
        return extension;
    }
    bool isReadOnly() const {
        return readOnly;
    }
    string& getOwner() {
        return owner;
    }
    string& getGroup() {
        return group;
    }
};
//Factory to manage shared formats (FLYWEIGHT)
class FilePropertiesFactory {
private:
    unordered_map<string, shared_ptr<FileProperties>> propertiesMap;
public:
    shared_ptr<FileProperties> getProperties(const string& ext, bool ro,
                                             const string& owner, const string& group) {
        string key = ext + (ro ? "T" : "F") + owner + group;
        auto it = propertiesMap.find(key);
        if (it != propertiesMap.end()) {
            return it->second;
        }
        auto props = make_shared<FileProperties>(ext, ro, owner, group);
        propertiesMap[key] = props;
        return props;
    }
    void clearUnused() {
        for (auto it = propertiesMap.begin(); it != propertiesMap.end(); ) {
            if (it->second.use_count() == 1) {
                it = propertiesMap.erase(it);
            } else {
                ++it;
            }
        }
    }
};
//Base interface for store items (VISITOR)
class Node {
public:
    virtual ~Node() = default;
    virtual void accept(class Visitor& visitor) = 0;
    virtual const string& getName() const = 0;
    virtual void printTree(const string& prefix, bool isLast) const = 0;
    virtual double getSize() const = 0;
};
class Directory; //forward declaration (VISITOR)
class Visitor {
public:
    virtual ~Visitor() = default;
    virtual void visit(class File& file) = 0;
    virtual void visit(Directory& dir) = 0;
};
class File : public Node {
private:
    string name;
    double size;
    shared_ptr<FileProperties> properties;
    Directory* parent;
public:
    File(string name, double size, shared_ptr<FileProperties> properties, Directory* parent) {
        this -> name = name;
        this -> size = size;
        this -> properties = properties;
        this -> parent = parent;
    }
    //accept method calls the VISITOR's visit method for a File
    void accept(Visitor& visitor) override {
        visitor.visit(*this);
    }
    const string& getName() const override {
        return name;
    }
    double getSize() const override {
        return size;
    }
    void printTree(const string& prefix, bool isLast) const override {
        if (isLast) {
            cout << prefix << "└── " << name << " (" << afterDOT(size) << "KB)" << endl;
        } else {
            cout << prefix << "├── " << name << " (" << afterDOT(size) << "KB)" << endl;
        }
    }
};
//VISITOR
class Directory : public Node {
private:
    string name;
    vector<unique_ptr<Node>> children;
    Directory* parent;
    int id;
public:
    Directory(int id, string name, Directory* parent = nullptr) {
        this -> id = id;
        this -> name = name;
        this -> parent = parent;
    }
    void addChild(unique_ptr<Node> child) {
        children.push_back(std::move(child));
    }
    //accept method calls the VISITOR's visit method for a Directory
    void accept(Visitor& visitor) override {
        visitor.visit(*this);
    }
    const string& getName() const override {
        return name;
    }
    double getSize() const override {
        double total = 0;
        for (const auto& child : children) {
            total += child->getSize();
        }
        return total;
    }
    void printTree(const string& prefix, bool isLast) const override {
        if (isLast) {
            cout << prefix << "└── " << name << endl;
        } else {
            cout << prefix << "├── " << name << endl;
        }
        string newPrefix;
        if (isLast) {
            newPrefix = prefix + "    ";
        } else {
            newPrefix = prefix + "│   ";
        }
        for (size_t i = 0; i < children.size(); ++i) {
            bool lastChild = (i == children.size() - 1);
            children[i]->printTree(newPrefix, lastChild);
        }
    }
    int getId() const {
        return id;
    }
    const vector<unique_ptr<Node>>& getChildren() const {
        return children;
    }
    //ITERATOR in Directory
    //also, I used DEPTH-First-Search (dfs) (to make my life easier, I took as an idea MY code from the DSA course (in coding exercises we already had the task about dfs/bfs))
    class Iterator {
    private:
        vector<const Node*> traversal;
        size_t current;
        void DFS(const Directory& dir) {
            for (const auto& child : dir.getChildren()) {
                traversal.push_back(child.get());
                if (auto subdir = dynamic_cast<const Directory*>(child.get())) {
                    DFS(*subdir);
                }
            }
        }
    public:
        Iterator(const Directory& dir) : current(0) {
            DFS(dir);
        }
        const Node* next() {
            if (current >= traversal.size()) {
                return nullptr;
            }
            return traversal[current++];
        }
        bool hasNext() const {
            return current < traversal.size();
        }
    };
    //in Lab solutions was the usage of unique pointer in ITERATOR. great, it works
    unique_ptr<Iterator> createIterator() const {
        return make_unique<Iterator>(*this);
    }
};
//VISITOR
class SizeVisitor : public Visitor {
private:
    double totalSize;
public:
    SizeVisitor() : totalSize(0) {}
    void visit(File& file) override {
        totalSize += file.getSize();
    }
    void visit(Directory& dir) override {
        for (const auto& child : dir.getChildren()) {
            child->accept(*this);
        }
    }
    double getTotalSize() const {
        return totalSize;
    }
};
//FLYWEIGHT, ITERATOR & VISITOR
class FileSystem {
private:
    unordered_map<int, Directory*> directories;
    unique_ptr<Directory> root;
    FilePropertiesFactory propertiesFactory;
public:
    ~FileSystem() {
        propertiesFactory.clearUnused();
    }
    FileSystem() {
        root = make_unique<Directory>(0, ".");
        directories[0] = root.get();
    }
    void createDirectory(int id, int parentId, const string& name) {
        // cout << id << parentId << name << endl;
        if (directories.find(id) == directories.end()) {
            auto newDir = make_unique<Directory>(id, name, directories[parentId]);
            directories[id] = newDir.get();
            directories[parentId]->addChild(std::move(newDir));
        } else {
            directories[id]->~Directory();
            new (directories[id]) Directory(id, name, directories[parentId]);
        }
    }
    void createFile(int parentId, bool readOnly, const string& owner, const string& group, double size, const string& fullName) {
        string ext = "";
        auto props = propertiesFactory.getProperties(ext, readOnly, owner, group);
        auto file = make_unique<File>(fullName, size, props, directories[parentId]);
        directories[parentId]->addChild(std::move(file));
    }
    void printTree() const {
        SizeVisitor sizeVisitor;
        root->accept(sizeVisitor);
        cout << "total: " << afterDOT(sizeVisitor.getTotalSize()) << "KB" << endl;
        cout << "." << endl;
        string prefix;
        const auto& children = root->getChildren();
        for (size_t i = 0; i < children.size(); ++i) {
            bool isLast = (i == children.size() - 1);
            children[i]->printTree(prefix, isLast);
        }
    }
};
//MAIN
int main() {
    FileSystem fileSys;
    int N;
    cin >> N;
    cin.ignore();
    for (int i = 0; i < N; ++i) {
        string line;
        getline(cin, line);
        istringstream input(line);
        string command;
        input >> command;
        //I had a problem with the test from the task, previous DIR parsing were working only if there was id AND parentId (like DIR 1234 project and DIR 1234 0 project)
        //it easier to work with strings to control this kind of issues
        if (command == "DIR") {
            int id;
            string name;
            //input >> id >> parentId;
            input >> id;
            int parentId = 0;
            if (input >> parentId) {
                getline(input, name);
            } else {
                input.clear();
                getline(input, name);
            }
            name.erase(0, name.find_first_not_of(" \n"));
            fileSys.createDirectory(id, parentId, name);
        } else if (command == "FILE") {
            int parentId;
            string r, owner, group, name;
            double size;
            input >> parentId >> r >> owner >> group >> size;
            getline(input, name);
            name.erase(0, name.find_first_not_of(" \n"));
            bool readOnly = (r == "T");
            fileSys.createFile(parentId, readOnly, owner, group, size, name);
        }
    }
    fileSys.printTree();
    printMemoryUsage();
    return 0;
}