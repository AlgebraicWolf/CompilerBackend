#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cctype>
#include <cstring>
#include <unistd.h>

#include "../Tree/Tree.h"

enum NODE_TYPE {
    D,
    DEF,
    VARLIST,
    ID,
    P,
    OP,
    C,
    B,
    IF,
    WHILE,
    E,
    ASSIGN,
    VAR,
    RETURN,
    CALL,
    ARITHM_OP,
    NUM,
    INPUT,
    OUTPUT,
    ADD,
    MUL,
    DIV,
    SUB,
    SQRT,
    BELOW,
    ABOVE,
    EQUAL
};

struct value_t {
    NODE_TYPE type;
    int id;
};

const char *input = "input.ast";
const char *output = "output.asm";

int currentVarDepth = 0;

char **IDs = nullptr;

void parseArgs(int argc, char *argv[]);

char *loadFile(const char *filename, size_t *fileSize);

tree_t *loadTree(const char *filename);

void BlockToASM(node_t *node, FILE *f, int *vars);

void ASTtoASM(tree_t *tree);

int ifCount = 0;
int whileCount = 0;

const int SPECIAL_SYMBOLS_LENGTH = 4;
const char specialSymbols[] = {'(', ')', ';', ','};

char *dumpNode(void *v) {
    value_t *value = (value_t *) v;

    char *buffer = (char *) calloc(1024, sizeof(char));
    switch (value->type) {
        case D:
            sprintf(buffer, "{ DEFINITION }");
            break;
        case OP:
            sprintf(buffer, "{ OPERATION }");
            break;
        case VARLIST:
            sprintf(buffer, "{ VARLIST }");
            break;
        case ID:
            sprintf(buffer, "{ ID } | %s", IDs[value->id]);
            break;
        case C:
            sprintf(buffer, "{ BRANCHING }");
            break;
        case B:
            sprintf(buffer, "{ BLOCK }");
            break;
        case DEF:
            sprintf(buffer, "{ FUNCTION }");
            break;
        case IF:
            sprintf(buffer, "{ IF }");
            break;
        case WHILE:
            sprintf(buffer, "{ WHILE }");
            break;
        case ASSIGN:
            sprintf(buffer, "{ = }");
            break;
        case VAR:
            sprintf(buffer, "{ VAR }");
            break;
        case RETURN:
            sprintf(buffer, "{ RETURN }");
            break;
        case CALL:
            sprintf(buffer, "{ CALL }");
            break;
        case BELOW:
            sprintf(buffer, "{ \\< }");
            break;

        case ABOVE:
            sprintf(buffer, "{ \\> }");
            break;

        case EQUAL:
            sprintf(buffer, "{ == }");
            break;

        case MUL:
            sprintf(buffer, "{ * }");
            break;

        case DIV:
            sprintf(buffer, "{ / }");
            break;

        case ADD:
            sprintf(buffer, "{ + }");
            break;

        case SUB:
            sprintf(buffer, "{ - }");
            break;

        case SQRT:
            sprintf(buffer, "{ sqrt }");
            break;
        case NUM:
            sprintf(buffer, "{ INTEGER: %d }", value->id);
            break;
        case INPUT:
            sprintf(buffer, "{ INPUT }");
            break;
        case OUTPUT:
            sprintf(buffer, "{ OUTPUT }");
            break;
    }

    return buffer;
}

int main(int argc, char *argv[]) {
    parseArgs(argc, argv);

    tree_t *ASTree = loadTree(input);
    treeDump(ASTree, "dump.dot", dumpNode);
    ASTtoASM(ASTree);
}

int *findVar(int *vars, int id) {
    assert(vars);
    while (*vars != id && *vars != -1)
        vars++;

    return vars;
}

void pushVarlist(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);
    assert(vars);

    if (node->right) {
        value_t *idVal = (value_t *) node->right->value;
        int *var = findVar(vars, idVal->id);

        if (*var != -1)
            fprintf(f, "push [0]\npop cx\npush [cx+%d]\n", var - vars + 1);
        else
            printf("ERROR! Undefined variable %s in function call!\n", IDs[idVal->id]);
    }
    if (node->left) {
        pushVarlist(node->left, f, vars);
    }
}

void executeExpression(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);
    assert(vars);

    value_t *op = (value_t *) node->value;

    int *var = nullptr;

    switch (op->type) {
        case ADD:
            executeExpression(node->left, f, vars);
            executeExpression(node->right, f, vars);
            fprintf(f, "add\n");
            break;
        case MUL:
            executeExpression(node->left, f, vars);
            executeExpression(node->right, f, vars);
            fprintf(f, "mul\n");
            break;
        case SUB:
            executeExpression(node->right, f, vars);
            executeExpression(node->left, f, vars);
            fprintf(f, "sub\n");
            break;
        case DIV:
            executeExpression(node->right, f, vars);
            executeExpression(node->left, f, vars);
            fprintf(f, "div\n");
            break;

        case EQUAL:
            executeExpression(node->right, f, vars);
            executeExpression(node->left, f, vars);
            fprintf(f, "call equal\n");
            break;

        case BELOW:
            executeExpression(node->right, f, vars);
            executeExpression(node->left, f, vars);
            fprintf(f, "call below\n");
            break;

        case ABOVE:
            executeExpression(node->right, f, vars);
            executeExpression(node->left, f, vars);
            fprintf(f, "call above\n");
            break;

        case SQRT:
            executeExpression(node->right, f, vars);
            fprintf(f, "sqrt\n");
            break;

        case ID:
            var = findVar(vars, op->id);
            if (*var != -1)
                fprintf(f, "push [0]\npop cx\npush [cx+%d]\n", var - vars + 1);
            else
                printf("ERROR! Undefined variable %s in expression!\n", IDs[op->id]);
            break;
        case NUM:
            fprintf(f, "push %d\n", op->id);
            break;
        case CALL: {
            value_t *funcData = (value_t *) node->left->value;

            pushVarlist(node->right, f, vars);

            fprintf(f, "push [0]\npush %d\nadd\npop [0]\n", currentVarDepth);
            fprintf(f, "call %s\n", IDs[funcData->id]);
            fprintf(f, "push %d\npush [0]\nsub\npop [0]\n", currentVarDepth);
        }
            break;
    }
}

void WhileToASM(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);

    node_t *condition = node->left;
    node_t *block = node->right;

    fprintf(f, "while%dcondition:\n", whileCount);
    executeExpression(condition, f, vars);
    fprintf(f, "push 0\njae while%dend\n\n", whileCount);
    BlockToASM(block, f, vars);

    fprintf(f, "jmp while%dcondition\nwhile%dend:", whileCount, whileCount);
    whileCount++;
}

void IfToASM(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);
    node_t *condition = node->left;
    node_t *branching = node->right;

    node_t *trueCase = branching->right;
    node_t *falseCase = branching->left;

    executeExpression(condition, f, vars);

    int currentCount = ifCount;
    ifCount++;

    if (falseCase)
        fprintf(f, "push 0\njae if%dfalse\nif%dtrue:\npop ax\npop ax\n", currentCount, currentCount);
    else
        fprintf(f, "push 0\njae if%dend\nif%dtrue:\npop ax\npop ax\n", currentCount, currentCount);

    BlockToASM(trueCase, f, vars);

    if (falseCase) {
        fprintf(f, "jmp if%dend\nif%dfalse:\npop ax\npop ax\n", currentCount, currentCount);
        BlockToASM(falseCase, f, vars);
    }

    fprintf(f, "if%dend:\n", currentCount);
}

void VarToASM(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);

    node_t *idNode = node->right;
    node_t *expression = node->left;

    if (expression) {

        value_t *idValue = (value_t *) idNode->value;

        int *var = findVar(vars, idValue->id);
        if (*var == -1) {
            printf("ERROR! Undefined variable %s in assignment!\n", IDs[idValue->id]);
        } else {
            executeExpression(expression, f, vars);
            fprintf(f, "push [0]\npop cx\npop [cx+%d]\n", var - vars + 1);
        }
    }
}

void AssignToASM(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);

    node_t *idNode = node->left;
    node_t *expression = node->right;


    value_t *idValue = (value_t *) idNode->value;

    int *var = findVar(vars, idValue->id);
    if (*var == -1) {
        printf("ERROR! Undefined variable %s in assignment!\n", IDs[idValue->id]);
    } else {
        executeExpression(expression, f, vars);
        fprintf(f, "push [0]\npop cx\npop [cx+%d]\n", var - vars + 1);
    }
}

void InputToASM(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);

    node_t *idNode = node->right;
    value_t *idValue = (value_t *) idNode->value;

    int *var = findVar(vars, idValue->id);
    if (*var == -1) {
        printf("ERROR! Undefined variable %s in input call!\n", IDs[idValue->id]);
    } else {
        fprintf(f, "in\npush [0]\npop cx\npop [cx+%d]\n", var - vars + 1);
    }
}

void OutputToASM(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);

    node_t *idNode = node->right;
    value_t *idValue = (value_t *) idNode->value;

    int *var = findVar(vars, idValue->id);
    if (*var == -1) {
        printf("ERROR! Undefined variable %s in output call!\n", IDs[idValue->id]);
    } else {
        fprintf(f, "push [0]\npop cx\npush [cx+%d]\nout\npop [cx+%d]\n", var - vars + 1, var - vars + 1);
    }
}

void ReturnToASM(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);

    node_t *idNode = node->right;
    value_t *idValue = (value_t *) idNode->value;

    int *var = findVar(vars, idValue->id);
    if (*var == -1) {
        printf("ERROR! Undefined variable %s in return call!\n", IDs[idValue->id]);
    } else {
        fprintf(f, "pop dx\npush [0]\npop cx\npush [cx+%d]\npush dx\nret\n", var - vars + 1, var - vars + 1);
    }
}

void OpToASM(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);

    node_t *operationNode = node->right;
    auto val = (value_t *) operationNode->value;

    switch (val->type) {
        case IF:
            IfToASM(operationNode, f, vars);
            break;

        case WHILE:
            WhileToASM(operationNode, f, vars);
            break;

        case VAR:
            VarToASM(operationNode, f, vars);
            break;

        case ASSIGN:
            AssignToASM(operationNode, f, vars);
            break;

        case INPUT:
            InputToASM(operationNode, f, vars);
            break;

        case OUTPUT:
            OutputToASM(operationNode, f, vars);
            break;

        case RETURN:
            ReturnToASM(operationNode, f, vars);
            break;
    }

    if (node->left)
        OpToASM(node->left, f, vars);
}

void BlockToASM(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);

    node_t *op = node->right;
    if (op)
        OpToASM(op, f, vars);
}

int countVars(node_t *node) {
    if (!node)
        return 0;

    value_t *val = (value_t *) node->value;
    if (val->type == VAR)
        return 1;

    else if (val->type == VARLIST && node->right)
        return 1 + countVars(node->left);

    else
        return countVars(node->left) + countVars(node->right);
}

void addVars(node_t *node, int *vars) {
    if (!node)
        return;

    value_t *val = (value_t *) node->value;

    if(val->type == VARLIST && !node->right)
        return;

    if (val->type == VAR || val->type == VARLIST) {
        node_t *id = node->right;
        value_t *idVal = (value_t *) id->value;

        int *current = findVar(vars, idVal->id);
        *current = idVal->id;

        if (val->type == VARLIST)
            addVars(node->left, vars);
    } else {
        addVars(node->left, vars);
        addVars(node->right, vars);
    }
}

int *getVariables(node_t *node) {
    assert(node);
    int count = countVars(node);
    auto vars = (int *) calloc(count + 1, sizeof(int));
    for (int i = 0; i < count + 1; i++)
        vars[i] = -1;

    addVars(node, vars);

    return vars;
}

void popParam(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);
    assert(vars);

    node_t *idNode = node->right;
    value_t *idVal = (value_t *) idNode->value;

    int *var = findVar(vars, idVal->id);

    if(node->left)
        popParam(node->left, f, vars);

    fprintf(f, "pop [cx+%d]\n", var - vars + 1);
}

void popVarlist(node_t *node, FILE *f, int *vars) {
    assert(node);
    assert(f);
    assert(vars);

    if(node->right) {
        fprintf(f, "pop dx\npush [0]\npop cx\n");
        popParam(node, f, vars);
        fprintf(f, "push dx\n");
    }
}

void DefToASM(node_t *node, FILE *f) {
    assert(node);
    assert(f);

    node_t *id = node->right;
    node_t *block = id->right;

    int *vars = getVariables(node);
    currentVarDepth = countVars(node);

    value_t *idValue = (value_t *) id->value;

    fprintf(f, "%s:\n", IDs[idValue->id]);
    popVarlist(node->left, f, vars);
    BlockToASM(block, f, vars);
    fprintf(f, "ret\n\n");
}

void DtoASM(node_t *node, FILE *f) {
    assert(node);
    assert(f);

    DefToASM(node->right, f);

    if (node->left)
        DtoASM(node->left, f);
}

void PtoASM(node_t *node, FILE *f) {
    assert(node);

    auto val = (value_t *) node->value;

    fprintf(f, "call main\nend\n\n");
    fprintf(f,
            "equal:\npop dx\nje eq1\njne eq0\neq1:\npush 1\njmp eq_end\neq0:\npush 0\neq_end:\npop ax\npop bx\npop bx\npush ax\npush dx\nret\n\n");
    fprintf(f,
            "below:\npop dx\njb b1\njae b0\nb1:\npush 1\njmp b_end\nb0:\npush 0\nb_end:\npop ax\npop bx\npop bx\npush ax\npush dx\nret\n\n");
    fprintf(f,
            "above:\npop dx\nja a1\njbe a0\na1:\npush 1\njmp a_end\na0:\npush 0\na_end:\npop ax\npop bx\npop bx\npush ax\npush dx\nret\n\n");

    DtoASM(node->right, f);
}

void ASTtoASM(tree_t *tree) {
    assert(tree);

    FILE *out = fopen(output, "w");
    PtoASM(tree->head, out);
    fclose(out);
}

NODE_TYPE getType(char *str) {
    assert(str);
    if (strcmp(str, "DECLARATION") == 0) {
        return D;
    } else if (strcmp(str, "IF") == 0) {
        return IF;
    } else if (strcmp(str, "WHILE") == 0) {
        return WHILE;
    } else if (strcmp(str, "FUNCTION") == 0) {
        return DEF;
    } else if (strcmp(str, "VARLIST") == 0) {
        return VARLIST;
    } else if (strcmp(str, "OP") == 0) {
        return OP;
    } else if (strcmp(str, "ASSIGN") == 0) {
        return ASSIGN;
    } else if (strcmp(str, "RETURN") == 0) {
        return RETURN;
    } else if (strcmp(str, "INITIALIZE") == 0) {
        return VAR;
    } else if (strcmp(str, "CALL") == 0) {
        return CALL;
    } else if (strcmp(str, "INPUT") == 0) {
        return INPUT;
    } else if (strcmp(str, "OUTPUT") == 0) {
        return OUTPUT;
    } else if (strcmp(str, "PROGRAM_ROOT") == 0) {
        return P;
    } else if (strcmp(str, "C") == 0) {
        return C;
    } else if (strcmp(str, "BLOCK") == 0) {
        return B;
    } else if (strcmp(str, "ADD") == 0) {
        return ADD;
    } else if (strcmp(str, "SUB") == 0) {
        return SUB;
    } else if (strcmp(str, "MUL") == 0) {
        return MUL;
    } else if (strcmp(str, "DIV") == 0) {
        return DIV;
    } else if (strcmp(str, "BELOW") == 0) {
        return BELOW;
    } else if (strcmp(str, "ABOVE") == 0) {
        return ABOVE;
    } else if (strcmp(str, "EQUAL") == 0) {
        return EQUAL;
    } else if (strcmp(str, "SQR") == 0) {
        return SQRT;
    } else {
        return ID;
    }
}

value_t *makeValue(NODE_TYPE type, int id) {
    auto val = (value_t *) calloc(1, (sizeof(value_t)));
    val->type = type;
    val->id = id;

    return val;
}

char *skipSpaces(char *str) {
    while (isblank(*str) && *str != '\0')
        str++;

    return str;
}

int getID(char *id) {
    assert(id);
    char **found = IDs;
    while (*found) {
        if (strcmp(id, *found))
            found++;
        else
            break;
    }
    if (!*found)
        *found = id;

    return found - IDs;
}

char *getString(char **tree) {
    assert(tree);
    assert(*tree);

    char *str = nullptr;
    int scanned = 0;
    sscanf(*tree, "%m[A-Za-z_]%n", &str, &scanned);
    (*tree) += scanned;

    return str;
}

int getNum(char **tree) {
    assert(tree);
    assert(*tree);

    int result = 0;
    int scanned = 0;
    sscanf(*tree, "%d%n", &result, &scanned);
    (*tree) += scanned;

    return result;
}

node_t *loadSubtree(char **tree) {
    assert(tree);
    *tree = skipSpaces(*tree);

    value_t *val = nullptr;
    node_t *node = nullptr;
    node_t *left = nullptr;
    node_t *right = nullptr;

    if (**tree == '@') {
        (*tree)++;
        return nullptr;
    } else if (isdigit(**tree)) {
        int number = getNum(tree);
        val = makeValue(NUM, number);
    } else {
        char *identifier = getString(tree);
        NODE_TYPE type = getType(identifier);
        int id = 0;
        if (type == ID) {
            id = getID(identifier);
        }
        val = makeValue(type, id);
    }

    *tree = skipSpaces(*tree);
    if (**tree == '{') { // kids are there
        (*tree)++;
        left = loadSubtree(tree);
        *tree = skipSpaces(*tree);

        if (**tree != '}')
            return nullptr;

        (*tree)++;

        *tree = skipSpaces(*tree);

        if (**tree != '{')
            return nullptr;
        (*tree)++;

        right = loadSubtree(tree);
        *tree = skipSpaces(*tree);

        if (**tree != '}')
            return nullptr;

        (*tree)++;
    }

    node = makeNode(nullptr, left, right, val);

    *tree = skipSpaces(*tree);

    if (**tree == '}') {
        return node;
    } else
        return nullptr;
}

tree_t *loadTree(const char *filename) {
    assert(filename);
    size_t size = 0;

    char *serialized = loadFile(filename, &size);
    IDs = (char **) calloc(size, sizeof(char));

    serialized = skipSpaces(serialized);
    if (*serialized != '{')
        return nullptr;

    serialized++;

    node_t *tree_head = loadSubtree(&serialized);

    if (*serialized != '}')
        return nullptr;

    tree_t *tree = makeTree(nullptr);

    tree->head = tree_head;
    return tree;
}

void parseArgs(int argc, char *argv[]) {
    int res = 0;
    while ((res = getopt(argc, argv, "i:o:")) != -1) {
        switch (res) {
            case 'i':
                input = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case '?':
                printf("Invalid argument found!\n");
                break;
        }
    }
}

size_t getFilesize(FILE *f) {
    assert(f);

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    return size;
}

char *loadFile(const char *filename, size_t *fileSize) {
    assert(filename);

    FILE *input = fopen(filename, "r");

    size_t size = getFilesize(input);
    if (fileSize)
        *fileSize = size;

    char *content = (char *) calloc(size, sizeof(char));
    fread(content, sizeof(char), size, input);

    fclose(input);

    return content;
}
