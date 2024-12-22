#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_IDENT_LENGTH 256
#define MAX_STRING_LENGTH 30
FILE* fp;
int currentLine = 1;
typedef enum {
  IDENTIFIER,
  INT_CONST,
  OPERATOR,
  STR_CONST,
  KEYWORD,
  ENDOFLINE,
  NEWLINE,
  NO_TYPE,
  ENDOFFILE,
  PARENTHESIS_OPEN,
  PARENTHESIS_CLOSE,
  COMMA,
  CURLY_OPEN,
  CURLY_CLOSE
} TokenType;

typedef enum {
  INT,
  TEXT
} DataType;

typedef struct {
  TokenType type;
  char* lexeme;
} Token;

typedef struct {
  char name[MAX_IDENT_LENGTH];
  char* value;
  DataType type;
} Variable;

Variable* variables;
size_t variablesSize = 0;

void raiseError(char* message)
{

  printf("ERR! Line %d:  %s\n", currentLine, message);;
  exit(1);
}

char skipWhitespace(char ch) {
  while (isspace(ch)) {
    ch = (char) fgetc(fp);
  }
  return ch;
}

char skipComment(char ch) {
  if (ch == '/') {
    ch = (char) fgetc(fp);
    char nextc;
    if (ch == '*') {
      do {
        if (nextc == EOF) {
          raiseError("Comment cannot be terminated!");
        }
        ch = nextc;
        nextc = (char) fgetc(fp);
      } while (!(ch == '*' && nextc == '/'));
      return (char) fgetc(fp);
    } else {
      raiseError("Unrecognized character: '/'");
    }
  }
  return ch;
}

bool isKeyword (char str[]) {
  const char* KEYWORDS[] = {"int", "loop", "text", "times", "read", "write","newLine","is"};
  for (int i = 0; i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); i++) {
    if (strcmp(KEYWORDS[i], str) == 0) {
      return true;
    }
  }
  return false;
}

char isOperator(char ch) {
  if (ch == '+' || ch == '-') { 
    return ch;
  }
  if (ch == 'i') {
    long int pos = ftell(fp);
    char nextCh = (char)fgetc(fp);
    if (nextCh == 's') {
      return '=';
    }
    fseek(fp, pos, SEEK_SET);
  }
  return '\0';
}

Token getNextToken() {
  Token token;
  token.lexeme = calloc(MAX_IDENT_LENGTH, sizeof(char));
  char ch = (char) fgetc(fp);

  //SKIP WHITESPACE and COMMENT
  while(isspace(ch) || ch == '/') {
    ch = skipComment(ch);
    ch = skipWhitespace(ch);
    if (ch == EOF) {
      token.type = ENDOFFILE;
      token.lexeme[0] = '\0';
      return token;
    }
  }
  if (ch == 't') {
      int next_char = fgetc(fp);
      if (next_char == 'i' && fgetc(fp) == 'm' && fgetc(fp) == 'e' && fgetc(fp) == 's') {
          token.type = ENDOFLINE;
          strcpy(token.lexeme, "");
          return token;
      }
      else {
          ungetc(next_char, fp);
      }
  }

  //ENDOFLINE
  if (ch == 'n' && fgetc(fp) == 'e' && fgetc(fp) == 'w' && fgetc(fp) == 'L' &&
      fgetc(fp) == 'i' && fgetc(fp) == 'n' && fgetc(fp) == 'e' && fgetc(fp) == '.') {
      token.type = NEWLINE;
      strcpy(token.lexeme, "");
      return token;
  }
  else
  {
    if (ch == '.')
    {
      token.type = ENDOFLINE;
      strcpy(token.lexeme, "");
      return token;
    }
  }
  //OPERATOR
  char operator = isOperator(ch);
  if (operator != '\0') {
    token.type = OPERATOR;
    token.lexeme[0] = operator;
    token.lexeme[1] = '\0';
    return token;
  }
  //IDENTIFIER
  if (isalpha(ch)) {
    int j = 0;
    while ((isalnum(ch) || ch == '_' && ch != '\0')) {
      token.lexeme[j++] = ch;
      if(j > MAX_IDENT_LENGTH) {
        char errMessage[57];
        sprintf(errMessage, "Identifiers must be smaller or equal than %d characters!", 57);
        raiseError(errMessage);
      }
      ch = (char) fgetc(fp);
    }
    ungetc(ch, fp);
    token.lexeme[j] = '\0';
    token.type = IDENTIFIER;
    if(isKeyword(token.lexeme)){
      token.type = KEYWORD;
    }
    return token;
  }

  //INTEGER
  if (isdigit(ch)) {
    unsigned long long value = 0;
    while (isdigit(ch)) {
      value = value * 10 + (ch - '0');
      if(value > 4294967295 ) {
        raiseError("Integer value is too big!");
      }
      ch = (char) fgetc(fp);
    }
    if (isalpha(ch) || ch == '_') {
      raiseError("Invalid identifier, identifiers cannot start with a number!");
    }
    ungetc(ch,fp);
    sprintf(token.lexeme, "%llu", value);
    token.type = INT_CONST;
    return token;
  }

  // CURLY BRACES
  if (ch == '{')
  {
    token.type = CURLY_OPEN;
    strcpy(token.lexeme, "{");
    return token;
  }
  if (ch == '}')
  {
    token.type = CURLY_CLOSE;
    strcpy(token.lexeme, "}");
    return token;
  }

  //PARENTHESIS_OPEN
  if (ch == '(') {
    token.type = PARENTHESIS_OPEN;
    strcpy(token.lexeme, "(");
    return token;
  }

  //PARENTHESIS_CLOSE
  if (ch == ')') {
    token.type = PARENTHESIS_CLOSE;
    strcpy(token.lexeme, ")");
    return token;
  }

  //COMMA
  if (ch == ',') {
    token.type = COMMA;
    strcpy(token.lexeme, ",");
    return token;
  }

  //STRING CONSTANT
  if (ch == '"') {
    int j = 0;
    ch = (char) fgetc(fp);
    while (ch != '"' && ch != '\0') {
        if (ch == EOF) {
            raiseError("String cannot be terminated!");
        }
        token.lexeme[j++] = ch;
        if (j >= MAX_STRING_LENGTH) {
            char errMessage[100];
            sprintf(errMessage, "String constants must be smaller than or equal to %d characters!", MAX_STRING_LENGTH);
            raiseError(errMessage);
        }
        ch = (char) fgetc(fp);
    }
    token.lexeme[j] = '\0';
    token.type = STR_CONST;
    return token;
}

  char errMessage[50];
  sprintf(errMessage, "Unrecognized character: '%c'!", ch);
  raiseError(errMessage);
}

// TUNA
void parseDeclaration(Token *line, int i) 
{
  if (line[0].type != KEYWORD || line[1].type != IDENTIFIER) {
    raiseError("Invalid variable initialization");
  }
  Variable variable;
  strcpy(variable.name, line[i].lexeme);
  variable.value = calloc(1, sizeof(char));
  variable.value = "";
  if (strcmp(line[0].lexeme, "int") == 0) {
    variable.type = INT;
  } else if (strcmp(line[0].lexeme, "text") == 0) {
    variable.type = TEXT;
  } else {
    char errMessage[100];
    sprintf(errMessage, "Unrecognized type: %s!", line[i].lexeme);
    raiseError(errMessage);
  }
  variables[variablesSize++] = variable;
}

Variable* getVariable(char *name) {
  for (int i = 0; i < variablesSize; i++)
  {
    if (strcmp(variables[i].name, name) == 0)
    {
      return &variables[i];
    }
  }
  char errMessage[50];
  return &variables[5];
}

void parseOutput(Token *line) {
  Variable variable = *getVariable(line[1].lexeme);
  FILE *fp = fopen("output.txt", "a");
  for (int i = 0; line[i].type != NO_TYPE; i++)
  {
    Variable variable = *getVariable(line[i].lexeme);
    if (variable.type != NO_TYPE && variable.value != NULL)
    {
      printf("%s\n", variable.value);
      fprintf(fp, "%s\n", variable.value);
    }
    if (line[i].type == STR_CONST && line[i].type != NO_TYPE && line[i].lexeme != NULL)
    {
      printf("%s\n", line[i].lexeme);
      fprintf(fp, "%s\n", line[i].lexeme);
    }
  }
}

void parseInput(Token *line) {
  if (line[1].type != STR_CONST|| line[2].type != COMMA || line[4].type != NO_TYPE) {
    raiseError("Invalid input!");
  }
  if (line[2].type != COMMA && strcmp(line[1].lexeme, "prompt") != 0) {
    raiseError("Invalid input!");
  }
  Variable prompt = *getVariable(line[3].lexeme);
  printf("%s %s", line[1].lexeme, prompt.value);
  Variable* variable = getVariable(line[1].lexeme);
  char buffer[100];
  fgets(buffer, 100, stdin);
  buffer[strcspn(buffer, "\n")] = 0;
  variable->value = calloc(strlen(buffer) + 1, sizeof(char));
  strcpy(variable->value, buffer);
  printf("%s\n", buffer);
}

void parseAssignment(Token *line, int i)
{
  if (line[0].type != IDENTIFIER && line[1].type != IDENTIFIER) {
    raiseError("Invalid assignment!");
  }
  Variable *variable = getVariable(line[0].lexeme);
  if (line[3].type == INT || line[3].type == TEXT)
  {
    Variable *variable = getVariable(line[1].lexeme);
  }
  if (line[2].type == INT_CONST){
    if (variable->type != INT) {
      raiseError("Invalid assignment!");
    }
    variable->value = calloc(strlen(line[2].lexeme) + 1, sizeof(char));
    strcpy(variable->value, line[2].lexeme);
  }
  else if (line[3].type == INT_CONST)
  {
    if (variable->type != INT)
    {
      raiseError("Invalid assignment!");
    }
    variable->value = calloc(strlen(line[3].lexeme) + 1, sizeof(char));
    strcpy(variable->value, line[3].lexeme);
  }
  else if (line[2].type == STR_CONST)
  {
    if (variable->type != TEXT) {
      raiseError("Invalid assignment!");
    }
    variable->value = calloc(strlen(line[2].lexeme) + 1, sizeof(char));
    strcpy(variable->value, line[2].lexeme);
  }
  else if (line[3].type == STR_CONST)
  {
    variable->value = calloc(strlen(line[3].lexeme) + 1, sizeof(char));
    strcpy(variable->value, line[3].lexeme);
  }
  else if (line[2].type == IDENTIFIER) 
  {
    Variable *variable2 = getVariable(line[2].lexeme);
    if (variable->type != variable2->type) {
      raiseError("Invalid assignment!");
    }
    variable->value = calloc(strlen(variable2->value) + 1, sizeof(char));
    strcpy(variable->value, variable2->value);
  } 
  else 
  {
    raiseError("Invalid assignment!");
  }
}

int sizeFunc(const char *string) {
  int i = 0;
  while (string[i] != '\0') {
    i++;
  }
  return i;
}

char* subsFunc(const char *string, int start, int end) {
  char *substring = calloc(end - start + 1, sizeof(char));
  int j = 0;
  for (int i = start; i < end; i++) {
    substring[j++] = string[i];
  }
  return substring;
}

void parseArithmeticAssignment(Token *line , int i) {
  if (line[0].type != IDENTIFIER || line[3].type != OPERATOR || line[5].type != NO_TYPE) {
    raiseError("Invalid arithmetic assignment!");
  }
  Variable *variable1 = getVariable(line[0].lexeme);
  if(variable1->type == INT) {
    int value1;
    int value2;
    if (line[2].type != INT_CONST && line[2].type != IDENTIFIER) {
      raiseError("Invalid  arithmetic assignment!");
    }
    if (line[4].type != INT_CONST && line[4].type != IDENTIFIER) {
      raiseError("Invalid arithmetic assignment!");
    }
    if (line[2].type == INT_CONST) {
      value1 = strtol(line[2].lexeme, NULL, 10);
    } else {
      Variable *variable2 = getVariable(line[2].lexeme);
      if (variable2->type != INT) {
        raiseError("Invalid arithmetic assignment!");
      }
      value1 = strtol(variable2->value, NULL, 10);
    }
    if (line[4].type == INT_CONST) {
      value2 = strtol(line[4].lexeme, NULL, 10);
    } else {
      Variable *variable2 = getVariable(line[4].lexeme);
      if (variable2->type != INT) {
        raiseError("Invalid arithmetic assignment!");
      }
      value2 = strtol(variable2->value, NULL, 10);
    }
    if (strcmp(line[3].lexeme, "+") == 0) {
      variable1->value = calloc(10, sizeof(char));
      sprintf(variable1->value, "%d", value1 + value2);
    } else if (strcmp(line[3].lexeme, "-") == 0) {
      variable1->value = calloc(10, sizeof(char));
      sprintf(variable1->value, "%d", value1 - value2);
      if (value1 - value2 < 0) {
        raiseError("The answer cannot be negative!");
      }
    } else {
      raiseError("Invalid arithmetic assignment!");
    }
  }

  if(variable1->type == TEXT) {
    char *value1;
    char *value2;
    if (line[2].type != STR_CONST && line[2].type != IDENTIFIER) {
      raiseError("Invalid arithmetic assignment!");
    }
    if (line[4].type != STR_CONST && line[4].type != IDENTIFIER) {
      raiseError("Invalid arithmetic assignment!");
    }
    if (line[2].type == STR_CONST) {
      value1 = line[2].lexeme;
    } else {
      Variable *variable2 = getVariable(line[2].lexeme);
      if (variable2->type != TEXT) {
        raiseError("Invalid arithmetic assignment!");
      }
      value1 = variable2->value;
    }
    if (line[4].type == STR_CONST) {
      value2 = line[4].lexeme;
    } else {
      Variable *variable2 = getVariable(line[4].lexeme);
      if (variable2->type != TEXT) {
        raiseError("Invalid arithmetic assignment!");
      }
      value2 = variable2->value;
    }
    if (strcmp(line[3].lexeme, "+") == 0) {
      variable1->value = calloc(strlen(value1) + strlen(value2) + 1, sizeof(char));
      strcpy(variable1->value, value1);
      strcat(variable1->value, value2);
    } else if (strcmp(line[3].lexeme, "-") == 0) {
      if (strlen(value1) < strlen(value2)) {
        raiseError("The subtrahend cannot be longer than the minuend!");
      }
      size_t resultLength = strlen(value1) - strlen(value2) + 1;
      variable1->value = calloc(resultLength, sizeof(char));
      char* found = strstr(value1, value2);
      if (found != NULL) {
        strncpy(variable1->value, value1, found - value1);
        strcat(variable1->value, found + strlen(value2));
      } else {
        strcpy(variable1->value, value1);
      }
    } else {
      raiseError("Invalid arithmetic assignment!");
    }
  }
}
//BURAK 
void parseLine(Token *line)
{
  // NEWLINE
  if (line[0].type == NEWLINE)
  {
    printf("\n");
    return ;
  }
  //DECLARATION
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "loop") == 0)
    return ;
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "int") == 0 || strcmp(line[0].lexeme, "text") == 0) {
    for (int j = 0; line[j].type != NO_TYPE; j++)
    {
      if (line[j].type == COMMA)
      {
        parseDeclaration(line, j + 1);
      }
      else if(line[3].type == OPERATOR && line[5].type == NO_TYPE)
      {
        return parseArithmeticAssignment(line, 1);
      }
      if (strcmp(line[j].lexeme, "=") == 0)
      {
        parseAssignment(line, j + 1);
      }
    }
    parseDeclaration(line, 1);
    return ;
  }

  //COMMAND WRITE
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "write") == 0) {
    return parseOutput(line);
  }
  //COMMAND READ
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "read") == 0) {
    return parseInput(line);
  }
  //ASSIGNMENT
  if (line[1].type == OPERATOR && strcmp(line[1].lexeme, "=") == 0)
  {
    if(line[3].type == NO_TYPE)
    {
      return parseAssignment(line, 1);
    }
    else if(line[3].type == OPERATOR && line[5].type == NO_TYPE)
    {
      return parseArithmeticAssignment(line, 1);
    }
    else if (strcmp(line[3].lexeme, "-") || strcmp(line[2].lexeme, "-"))
    {
      raiseError("Negative values are not allowed!");
    }
    else
    {
      raiseError("Invalid assignment!");
    }
  }
  raiseError("Parsing error!");
}

int main(int argc, char *argv[]) {
  
  variables = calloc(10, sizeof(Variable));
  char* file = "testCases.sta";
  long int file_location;
  long int loop_location;
  if(argc > 1) {
      file = argv[1];
  }

  fp = fopen(file, "r");

  if(fp == NULL) {
    printf("Cannot open file: %s\n", file);
    return 1;
  }

  Token token;
  char c = (char) fgetc(fp);
  Token* line = calloc(100, sizeof(Token));
  int i = 0;
  int loopCount = 1;
  int count = 0;
  int flag = 0;
  while (c != EOF)
  {
    ungetc(c, fp);
    token = getNextToken();
    if (token.type == CURLY_OPEN)
    {
      flag = 0;
      file_location = ftell(fp);
    }
    if (token.type != ENDOFLINE && token.type != ENDOFFILE && token.type != CURLY_OPEN && token.type != CURLY_CLOSE) 
    {
      line[i++] = token;
    }
    else if (token.type == ENDOFLINE) 
    {
      line[i].type = NO_TYPE;
      if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "loop") == 0)
      {
        if (line[1].type != INT_CONST)
        {
          if (flag == 0)
          {
            flag = 1;
            loop_location = ftell(fp);
          }
          Variable *variables = getVariable(line[1].lexeme);
          loopCount *= strtol(variables->value, NULL, 10);
        }
        else if (line[1].type == INT_CONST)
        {
          if (flag == 0)
          {
            flag = 1;
            loop_location = ftell(fp);
          }
          loopCount *= strtol(line[1].lexeme, NULL, 10);
        }
        else
        {
          raiseError("Invalid loop statement!");
        }
      }
      parseLine(line);
      line = calloc(10, sizeof(Token));
      i = 0;     
      currentLine++;
    }
    if (flag == 1 && token.type == CURLY_CLOSE && count < loopCount)
    {
      fseek(fp, loop_location, SEEK_SET);
      count++;
    }
    if ((token.type == CURLY_CLOSE) && count < loopCount && flag == 0)
    {
      fseek(fp, file_location, SEEK_SET); 
      count++;
    }
    c = (char) fgetc(fp);
  }
}