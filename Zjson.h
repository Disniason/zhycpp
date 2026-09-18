#ifndef Z_JSON_H
#define Z_JSON_H
#pragma once

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <limits>
#include <type_traits>
#include <exception>
#include <cctype>
#include <cstddef>
#include <cstdint>

namespace zhy{

    class json{

        public:

        /*enum class json_token_id{
            LT_STRING,
            LT_NUMBER,
            LT_TRUE,
            LT_FALSE,
            LT_NULL,

            SE_OBJECTLEFT,//{
            SE_OBJECTRIGHT,//}
            SE_ARRAYLEFT,//[
            SE_ARRAYRIGHT,//]
            SE_COLON,//:
            SE_COMMA,//,

            TOKEN_ERROR

        };*/

        enum class json_token_id{
            TOKEN_ERROR,

            SE_OBJECTLEFT,//{
            SE_ARRAYLEFT,//[

            SE_COMMA,//,
            LT_STRING,
            SE_COLON,//:
            LT_NUMBER,
            LT_TRUE,
            LT_FALSE,
            LT_NULL,

            SE_ARRAYRIGHT,//]
            SE_OBJECTRIGHT,//}
        };

        class json_token{

            public:

            json_token_id tokenId;
            std::string tokenWord;

            json_token():tokenId(json_token_id::TOKEN_ERROR),tokenWord(){}
            ~json_token(){}

            json_token(const json_token & other):tokenId(other.tokenId),tokenWord(other.tokenWord){}
            json_token & operator =(const json_token & other){
                if (this != &other){
                    tokenId = other.tokenId;
                    tokenWord = other.tokenWord;
                }
                return *this;
            }

            inline static int hex2int(char c){
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                return -1;
            }

            inline static bool parse_uXXXX(const std::string & s,size_t pos,uint16_t & code){
                if (pos + 4 > s.size()) return false;
                int v = 0;
                for(int i=0;i<4;++i){
                    int d = hex2int(s[pos + i]);
                    if (d < 0) return false;
                    v = (v << 4) | d;
                }
                code = static_cast <uint16_t>(v);
                return true;
            }

            inline static void utf16_bmp_to_utf8(uint16_t cp,std::string & out){
                if (cp <= 0x007F){
                    out.push_back(static_cast<char>(cp));
                }
                else if (cp <= 0x07FF){
                    out.push_back(0xC0 | ((cp >> 6) & 0x1F));
                    out.push_back(0x80 | (cp & 0x3F));
                }
                else{
                    out.push_back(0xE0 | ((cp >> 12) & 0x0F));
                    out.push_back(0x80 | ((cp >> 6) & 0x3F));
                    out.push_back(0x80 | (cp & 0x3F));
                }
                return;
            }

            inline static std::string getStringValue(const std::string & str){
                std::string newStr;
                for(size_t i=1;i<(str.size() - 1);++i){
                    if (str[i] == '\\'){
                        switch (str[i + 1]){
                            case '\"':{newStr += '\"';break;}
                            case '\\':{newStr += '\\';break;}
                            case '/':{newStr += '/';break;}
                            case 'b':{newStr += '\b';break;}
                            case 'f':{newStr += '\f';break;}
                            case 'n':{newStr += '\n';break;}
                            case 'r':{newStr += '\r';break;}
                            case 't':{newStr += '\t';break;}
                            case 'u':{
                                uint16_t code;
                                if (parse_uXXXX(str,i + 2,code)){
                                    utf16_bmp_to_utf8(code,newStr);
                                    i += 4;
                                }
                                else{
                                    std::string msg = "zhy::json::json_token::getStringValue : unexpected Unicode escape " + str.substr(i,6);
                                    throw std::runtime_error(msg);
                                    newStr += "\\u";
                                }
                                break;
                            }
                            default:{
                                newStr += str[i];
                                newStr += str[i + 1];
                            }
                        }
                        i += 1;
                    }
                    else newStr += str[i];
                }
                return newStr;
            }

            template <class T>
            T getValue() const{
                if constexpr (std::is_same_v <std::decay_t <T>,std::string>){
                    if (tokenId == json_token_id::LT_STRING) return getStringValue(tokenWord);
                }
                else if constexpr (std::is_same_v <std::decay_t <T>,long double>){
                    if (tokenId == json_token_id::LT_NUMBER) return std::stold(tokenWord);
                }
                else if constexpr (std::is_same_v <std::decay_t <T>,bool>){
                    if (tokenId == json_token_id::LT_TRUE) return true;
                    else return false;
                }
                else{
                    std::string msg = "zhy::json::json_token::getValue : unexpected value type";
                    throw std::runtime_error(msg);
                }
                return T{};
            }

            void clear(){
                tokenId = json_token_id::TOKEN_ERROR;
                tokenWord.clear();
                return;
            }

            bool empty() const{
                return (
                    tokenId == json_token_id::TOKEN_ERROR &&
                    tokenWord.empty()
                );
            }

            void display(std::ostream & out = std::cout) const{
                out<<tokenWord<<" : ";
                auto it = tokenDisplayWord.find(tokenId);
                if (it == tokenDisplayWord.end()) out<<tokenDisplayWord.at(json_token_id::TOKEN_ERROR)<<std::endl;
                else out<<it->second<<std::endl;
                return;
            }

            bool isBoolean() const{
                return (
                    tokenId == json_token_id::LT_TRUE ||
                    tokenId == json_token_id::LT_FALSE
                );
            }

            inline static bool isHexadecimalChar(char ch){
                return (
                    (ch >= '0' && ch <= '9') ||
                    (ch >= 'a' && ch <= 'f') ||
                    (ch >= 'A' && ch <= 'F')
                );
            }

            inline static bool isWhiteSpaceChar(char input = ' '){//判断空白符
                return (
                    std::isspace(static_cast <unsigned char>(input)) ||
                    input == ' ' ||
                    input == '\t' ||
                    input == '\n' ||
                    input == '\v' ||
                    input == '\f' ||
                    input == '\r'
                );
            }


        };

        inline static const std::unordered_map <json_token_id,std::string> tokenDisplayWord = {
            {json_token_id::LT_STRING,"literal(string)"},
            {json_token_id::LT_NUMBER,"literal(number)"},
            {json_token_id::LT_TRUE,"literal(true)"},
            {json_token_id::LT_FALSE,"literal(false)"},
            {json_token_id::LT_NULL,"literal(null)"},

            {json_token_id::SE_OBJECTLEFT,"separator({)"},
            {json_token_id::SE_OBJECTRIGHT,"separator(})"},
            {json_token_id::SE_ARRAYLEFT,"separator([)"},
            {json_token_id::SE_ARRAYRIGHT,"separator(])"},
            {json_token_id::SE_COLON,"separator(:)"},
            {json_token_id::SE_COMMA,"separator(,)"},

            {json_token_id::TOKEN_ERROR,"unknown token"}
        };

        enum class lexer_state{
            START,

            STRING_IN,
            STRING_ESCAPE,
            STRING_ESCAPE_UNICODE,
            STRING_END,

            NUMBER_INT_IN,
            NUMBER_INT_END,
            NUMBER_INT_ZERO,
            NUMBER_POINT_IN,
            NUMBER_POINT_END,
            NUMBER_EXP_START,
            NUMBER_EXP_IN,
            NUMBER_EXP_END,

            TRUE_T,
            TRUE_R,
            TRUE_U,
            TRUE_E,

            FALSE_F,
            FALSE_A,
            FALSE_L,
            FALSE_S,
            FALSE_E,

            NULL_N,
            NULL_U,
            NULL_L,
            NULL_END,

            SE_END

        };

        inline static bool isStateEnd(lexer_state state){
            return (
                state == lexer_state::STRING_END ||

                state == lexer_state::NUMBER_INT_END ||
                state == lexer_state::NUMBER_INT_ZERO ||
                state == lexer_state::NUMBER_POINT_END ||
                state == lexer_state::NUMBER_EXP_END ||

                state == lexer_state::TRUE_E ||
                state == lexer_state::FALSE_E ||
                state == lexer_state::NULL_END ||

                state == lexer_state::SE_END
            );
        }

        private:

        inline static bool getToken(
            json_token & tempToken,
            const std::string & filePath = "stdin",
            std::istream & in = std::cin
        ){
            char tempChar;
            lexer_state tempState = lexer_state::START;
            std::string tempWord;
            unsigned short int unicodeCount = 0;
            bool inputOK = true;
            while (inputOK){
                switch (tempState){
                    case lexer_state::START:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar >= '1' && tempChar <= '9'){
                            tempState = lexer_state::NUMBER_INT_END;
                            tempWord += tempChar;
                            continue;
                        }
                        else if (json_token::isWhiteSpaceChar(tempChar)) continue;
                        switch (tempChar){
                            case '\"':{//字符串开始
                                tempState = lexer_state::STRING_IN;
                                tempWord += tempChar;
                                continue;
                            }
                            case '-':{
                                tempState = lexer_state::NUMBER_INT_IN;
                                tempWord += tempChar;
                                continue;
                            }
                            case '0':{
                                tempState = lexer_state::NUMBER_INT_ZERO;
                                tempWord += tempChar;
                                continue;
                            }
                            case 't':{
                                tempState = lexer_state::TRUE_T;
                                tempWord += tempChar;
                                continue;
                            }
                            case 'f':{
                                tempState = lexer_state::FALSE_F;
                                tempWord += tempChar;
                                continue;
                            }
                            case 'n':{
                                tempState = lexer_state::NULL_N;
                                tempWord += tempChar;
                                continue;
                            }
                            case '{':{
                                tempState = lexer_state::SE_END;
                                tempToken.tokenId = json_token_id::SE_OBJECTLEFT;
                                tempToken.tokenWord = "{";
                                return true;
                            }
                            case '}':{
                                tempState = lexer_state::SE_END;
                                tempToken.tokenId = json_token_id::SE_OBJECTRIGHT;
                                tempToken.tokenWord = "}";
                                return true;
                            }
                            case '[':{
                                tempState = lexer_state::SE_END;
                                tempToken.tokenId = json_token_id::SE_ARRAYLEFT;
                                tempToken.tokenWord = "[";
                                return true;
                            }
                            case ']':{
                                tempState = lexer_state::SE_END;
                                tempToken.tokenId = json_token_id::SE_ARRAYRIGHT;
                                tempToken.tokenWord = "]";
                                return true;
                            }
                            case ':':{
                                tempState = lexer_state::SE_END;
                                tempToken.tokenId = json_token_id::SE_COLON;
                                tempToken.tokenWord = ":";
                                return true;
                            }
                            case ',':{
                                tempState = lexer_state::SE_END;
                                tempToken.tokenId = json_token_id::SE_COMMA;
                                tempToken.tokenWord = ",";
                                return true;
                            }
                            default:{
                                std::string msg = "zhy::json::getToken : unexpected start character " + std::string(1,tempChar) + " of a token in file " + filePath;
                                throw std::runtime_error(msg);
                                continue;
                            }
                        }
                    }
                    case lexer_state::STRING_IN:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (inputOK) tempWord += tempChar;
                        else break;
                        switch (tempChar){
                            case '\\':{
                                tempState = lexer_state::STRING_ESCAPE;
                                continue;
                            }
                            case '\"':{
                                tempState = lexer_state::STRING_END;
                                tempToken.tokenId = json_token_id::LT_STRING;
                                tempToken.tokenWord = tempWord;
                                return true;
                            }
                            default:continue;
                        }
                    }
                    case lexer_state::STRING_ESCAPE:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (inputOK) tempWord += tempChar;
                        else break;
                        switch (tempChar){
                            case '\"':{
                                tempState = lexer_state::STRING_IN;
                                continue;
                            }
                            case '\\':{
                                tempState = lexer_state::STRING_IN;
                                continue;
                            }
                            case '/':{
                                tempState = lexer_state::STRING_IN;
                                continue;
                            }
                            case 'b':{
                                tempState = lexer_state::STRING_IN;
                                continue;
                            }
                            case 'f':{
                                tempState = lexer_state::STRING_IN;
                                continue;
                            }
                            case 'n':{
                                tempState = lexer_state::STRING_IN;
                                continue;
                            }
                            case 'r':{
                                tempState = lexer_state::STRING_IN;
                                continue;
                            }
                            case 't':{
                                tempState = lexer_state::STRING_IN;
                                continue;
                            }
                            case 'u':{
                                tempState = lexer_state::STRING_ESCAPE_UNICODE;
                                unicodeCount = 0;
                                continue;
                            }
                            default:{
                                tempState = lexer_state::STRING_IN;
                                std::string msg = "zhy::json::getToken : unexpected escape exists near token " + tempWord + " in file " + filePath;
                                throw std::runtime_error(msg);
                                continue;
                            }
                        }
                    }
                    case lexer_state::STRING_ESCAPE_UNICODE:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (inputOK) tempWord += tempChar;
                        else break;
                        if (json_token::isHexadecimalChar(tempChar)){
                            unicodeCount += 1;
                            if (unicodeCount >= 4){
                                tempState = lexer_state::STRING_IN;
                                unicodeCount = 0;
                            }
                            continue;
                        }
                        else{
                            tempState = lexer_state::STRING_IN;
                            unicodeCount = 0;
                            std::string msg = "zhy::json::getToken : unexpected Unicode escape character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::NUMBER_INT_IN:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == '0'){
                            tempState = lexer_state::NUMBER_INT_ZERO;
                            tempWord += tempChar;
                            continue;
                        }
                        else if (tempChar >= '1' && tempChar <= '9'){
                            tempState = lexer_state::NUMBER_INT_END;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::NUMBER_INT_END:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar >= '0' && tempChar <= '9'){
                            tempWord += tempChar;
                            continue;
                        }
                        else if (tempChar == '.'){
                            tempState = lexer_state::NUMBER_POINT_IN;
                            tempWord += tempChar;
                            continue;
                        }
                        else if (tempChar == 'e' || tempChar == 'E'){
                            tempState = lexer_state::NUMBER_EXP_START;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            tempToken.tokenId = json_token_id::LT_NUMBER;
                            tempToken.tokenWord = tempWord;
                            return true;
                        }
                    }
                    case lexer_state::NUMBER_INT_ZERO:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == '.'){
                            tempState = lexer_state::NUMBER_POINT_IN;
                            tempWord += tempChar;
                            continue;
                        }
                        else if (tempChar == 'e' || tempChar == 'E'){
                            tempState = lexer_state::NUMBER_EXP_START;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            tempToken.tokenId = json_token_id::LT_NUMBER;
                            tempToken.tokenWord = tempWord;
                            return true;
                        }
                    }
                    case lexer_state::NUMBER_POINT_IN:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar >= '0' && tempChar <= '9'){
                            tempState = lexer_state::NUMBER_POINT_END;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::NUMBER_POINT_END:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar >= '0' && tempChar <= '9'){
                            tempWord += tempChar;
                            continue;
                        }
                        else if (tempChar == 'e' || tempChar == 'E'){
                            tempState = lexer_state::NUMBER_EXP_START;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            tempToken.tokenId = json_token_id::LT_NUMBER;
                            tempToken.tokenWord = tempWord;
                            return true;
                        }
                    }
                    case lexer_state::NUMBER_EXP_START:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == '+' || tempChar == '-'){
                            tempState = lexer_state::NUMBER_EXP_IN;
                            tempWord += tempChar;
                            continue;
                        }
                        else if (tempChar >= '0' && tempChar <= '9'){
                            tempState = lexer_state::NUMBER_EXP_END;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::NUMBER_EXP_IN:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar >= '0' && tempChar <= '9'){
                            tempState = lexer_state::NUMBER_EXP_END;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::NUMBER_EXP_END:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar >= '0' && tempChar <= '9'){
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            tempToken.tokenId = json_token_id::LT_NUMBER;
                            tempToken.tokenWord = tempWord;
                            return true;
                        }
                    }
                    case lexer_state::TRUE_T:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == 'r'){
                            tempState = lexer_state::TRUE_R;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::TRUE_R:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == 'u'){
                            tempState = lexer_state::TRUE_U;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::TRUE_U:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == 'e'){
                            tempState = lexer_state::TRUE_E;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::TRUE_E:{
                        tempToken.tokenId = json_token_id::LT_TRUE;
                        tempToken.tokenWord = tempWord;
                        return true;
                    }
                    case lexer_state::FALSE_F:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == 'a'){
                            tempState = lexer_state::FALSE_A;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::FALSE_A:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == 'l'){
                            tempState = lexer_state::FALSE_L;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::FALSE_L:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == 's'){
                            tempState = lexer_state::FALSE_S;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::FALSE_S:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == 'e'){
                            tempState = lexer_state::FALSE_E;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::FALSE_E:{
                        tempToken.tokenId = json_token_id::LT_FALSE;
                        tempToken.tokenWord = tempWord;
                        return true;
                    }
                    case lexer_state::NULL_N:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == 'u'){
                            tempState = lexer_state::NULL_U;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::NULL_U:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == 'l'){
                            tempState = lexer_state::NULL_L;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::NULL_L:{
                        inputOK = static_cast <bool>(in.get(tempChar));
                        if (!inputOK) break;
                        if (tempChar == 'l'){
                            tempState = lexer_state::NULL_END;
                            tempWord += tempChar;
                            continue;
                        }
                        else{
                            in.unget();//回滚
                            std::string msg = "zhy::json::getToken : unexpected character exists near token " + tempWord + " in file " + filePath;
                            throw std::runtime_error(msg);
                            continue;
                        }
                    }
                    case lexer_state::NULL_END:{
                        tempToken.tokenId = json_token_id::LT_TRUE;
                        tempToken.tokenWord = tempWord;
                        return true;
                    }
                }
            }
            if (!isStateEnd(tempState)){
                if (tempState == lexer_state::START) return inputOK;
                std::string msg = "zhy::json::getToken : imperfect token " + tempWord + " exists in file " + filePath;
                throw std::runtime_error(msg);
                return inputOK;
            }
            return inputOK;
        }



        public:

        inline static void lexer_tester(
            const std::string & filePath,
            std::ostream & out = std::cout,
            std::ostream & err = std::cerr
        ){
            std::ifstream in(std::filesystem::path(filePath),std::ios::in);
            if (!in.is_open()){
                err<<"Failed to open file "<<filePath<<std::endl;
                return;
            }
            json_token tempToken;
            while (true){
                try{
                    if (getToken(tempToken,filePath,in)) tempToken.display(out);
                    else break;
                }
                catch (const std::exception & e){
                    err<<"lexical analysis exception :"<<std::endl;
                    err<<e.what()<<std::endl;
                    continue;
                }
            }
            in.close();
            return;
        }

        enum class json_value_id{
            OB_STRING,
            OB_NUMBER,
            OB_BOOLEAN,
            OB_NULL,
            OB_OBJECT,
            OB_ARRAY,

            OB_ERROR
        };

        class json_value;

        using objectValueType = std::unordered_map <std::string,std::shared_ptr <json_value>>;
        using arrayValueType = std::vector <std::shared_ptr <json_value>>;
        using valueType = std::variant <short int,std::string,long double,bool,std::nullptr_t,
            objectValueType,
            arrayValueType
        >;

        class json_value : public std::enable_shared_from_this <json_value>{

            public:

            json_value(){}
            virtual ~json_value() = default;

            json_value(const json_value & other) = default;
            json_value & operator =(const json_value & other) = default;

            virtual std::shared_ptr <json_value> clone() const = 0;

            /*virtual json_value_id getFlag() const{
                return json_value_id::OB_ERROR;
            }*/

            virtual json_value_id getFlag() const = 0;

            virtual valueType getValue() const = 0;

            virtual void print(size_t table,size_t width = 4,std::ostream & out = std::cout) const = 0;

            friend std::ostream & operator <<(std::ostream & out,const json_value & self){
                if (self.isJson()) self.print(0,4,out);
                else self.print(0,0,out);
                return out;
            }

            bool isString() const{
                return getFlag() == json_value_id::OB_STRING;
            }

            bool isNumber() const{
                return getFlag() == json_value_id::OB_NUMBER;
            }

            bool isBoolean() const{
                return getFlag() == json_value_id::OB_BOOLEAN;
            }

            bool isNull() const{
                return getFlag() == json_value_id::OB_NULL;
            }

            bool isObject() const{
                return getFlag() == json_value_id::OB_OBJECT;
            }

            bool isArray() const{
                return getFlag() == json_value_id::OB_ARRAY;
            }

            bool isJson() const{
                json_value_id flag = getFlag();
                return (
                    flag == json_value_id::OB_OBJECT ||
                    flag == json_value_id::OB_ARRAY
                );
            }

            bool isError() const{
                return getFlag() == json_value_id::OB_ERROR;
            }

            virtual size_t size() const = 0;

            virtual void clear() = 0;

            virtual bool equals(const std::shared_ptr <json_value> & other) const = 0;

        };

        class json_string : public json_value{

            public:

            std::string value;

            json_string():json_value(),value(){}
            json_string(const std::string & v):json_value(),value(v){}
            ~json_string() override = default;

            json_string(const json_string & other):json_value(other),value(other.value){}

            json_string & operator =(const json_string & other){
                if (this == &other) return *this;
                json_value::operator =(other);
                value = other.value;
                return *this;
            }

            std::shared_ptr <json_value> clone() const override{
                return std::make_shared <json_string>(*this);
            }

            json_value_id getFlag() const override{
                return json_value_id::OB_STRING;
            }

            valueType getValue() const override{
                return value;
            }

            void print(size_t table,size_t width = 4,std::ostream & out = std::cout) const override{
                out<<"\""<<value<<"\"";
                return;
            }

            size_t size() const override{
                return value.size();
            }

            void clear() override{
                value.clear();
                return;
            }

            bool equals(const std::shared_ptr <json_value> & other) const override{
                if (this->getFlag() != other->getFlag()) return false;
                else if (this->shared_from_this() == other) return true;
                else{
                    auto o = std::dynamic_pointer_cast <json_string>(other);
                    if (!o) return false;
                    return this->value == o->value;
                }
            }


        };

        class json_number : public json_value{

            public:

            long double value;

            json_number():json_value(),value(0.0){}
            json_number(long double v):json_value(),value(v){}
            ~json_number() override = default;

            json_number(const json_number & other):json_value(other),value(other.value){}

            json_number & operator =(const json_number & other){
                if (this == &other) return *this;
                json_value::operator =(other);
                value = other.value;
                return *this;
            }

            std::shared_ptr <json_value> clone() const override{
                return std::make_shared <json_number>(*this);
            }

            json_value_id getFlag() const override{
                return json_value_id::OB_NUMBER;
            }

            valueType getValue() const override{
                return value;
            }

            void print(size_t table,size_t width = 4,std::ostream & out = std::cout) const override{
                out<<value;
                return;
            }

            size_t size() const override{
                return std::numeric_limits <size_t>::max();
            }

            void clear() override{
                value = 0.0;
                return;
            }

            bool equals(const std::shared_ptr <json_value> & other) const override{
                if (this->getFlag() != other->getFlag()) return false;
                else if (this->shared_from_this() == other) return true;
                else{
                    auto o = std::dynamic_pointer_cast <json_number>(other);
                    if (!o) return false;
                    return this->value == o->value;
                }
            }


        };

        class json_boolean : public json_value{

            public:

            bool value;

            json_boolean():json_value(),value(false){}
            json_boolean(bool v):json_value(),value(v){}
            ~json_boolean() override = default;

            json_boolean(const json_boolean & other):json_value(other),value(other.value){}

            json_boolean & operator =(const json_boolean & other){
                if (this == &other) return *this;
                json_value::operator =(other);
                value = other.value;
                return *this;
            }

            std::shared_ptr <json_value> clone() const override{
                return std::make_shared <json_boolean>(*this);
            }

            json_value_id getFlag() const override{
                return json_value_id::OB_BOOLEAN;
            }

            valueType getValue() const override{
                return value;
            }

            void print(size_t table,size_t width = 4,std::ostream & out = std::cout) const override{
                out<<(value ? "true" : "false");
                return;
            }

            size_t size() const override{
                return std::numeric_limits <size_t>::max();
            }

            void clear() override{
                value = false;
                return;
            }

            bool equals(const std::shared_ptr <json_value> & other) const override{
                if (this->getFlag() != other->getFlag()) return false;
                else if (this->shared_from_this() == other) return true;
                else{
                    auto o = std::dynamic_pointer_cast <json_boolean>(other);
                    if (!o) return false;
                    return this->value == o->value;
                }
            }


        };

        class json_null : public json_value{

            public:

            json_null():json_value(){}
            ~json_null() override = default;

            json_null(const json_null & other):json_value(other){}

            json_null & operator =(const json_null & other){
                if (this == &other) return *this;
                json_value::operator =(other);
                return *this;
            }

            std::shared_ptr <json_value> clone() const override{
                return std::make_shared <json_null>(*this);
            }

            json_value_id getFlag() const override{
                return json_value_id::OB_NULL;
            }

            valueType getValue() const override{
                return nullptr;
            }

            void print(size_t table,size_t width = 4,std::ostream & out = std::cout) const override{
                out<<"null";
                return;
            }

            size_t size() const override{
                return std::numeric_limits <size_t>::max();
            }

            void clear() override{return;}

            bool equals(const std::shared_ptr <json_value> & other) const override{
                return this->getFlag() == other->getFlag();
            }


        };

        class json_object : public json_value{

            public:

            objectValueType value;

            json_object():json_value(),value(){}
            //json_object(const objectValueType & v):json_value(),value(v){}
            json_object(objectValueType && v):json_value(),value(std::move(v)){}
            ~json_object() override = default;

            json_object(const json_object & other):json_value(other){//深拷贝
                for(const auto & element : other.value){
                    value.emplace(element.first,element.second->clone());
                }
            }

            json_object & operator =(const json_object & other){
                if (this == &other) return *this;
                json_value::operator =(other);
                for(const auto & element : other.value){
                    value.emplace(element.first,element.second->clone());
                }
                return *this;
            }

            std::shared_ptr <json_value> clone() const override{
                return std::make_shared <json_object>(*this);
            }

            json_value_id getFlag() const override{
                return json_value_id::OB_OBJECT;
            }

            inline static json_value_id flag(){
                return json_value_id::OB_OBJECT;
            }

            valueType getValue() const override{
                return value;
            }

            void print(size_t table,size_t width = 4,std::ostream & out = std::cout) const override{
                if (value.empty()){
                    out<<"{}";
                    return;
                }
                std::string tab(table * width,' ');
                std::string newTab((table + 1) * width,' ');
                out<<"{";
                if (width != 0) out<<std::endl;
                auto element = value.begin();
                if (element->second != nullptr){
                    out<<newTab<<"\""<<element->first<<"\": ";
                    if (element->second->getFlag() < json_value_id::OB_OBJECT) out<<*(element->second);
                    else element->second->print(table + 1,width,out);
                }
                ++element;
                for(;element != value.end();++element){
                    if (element->second == nullptr) continue;
                    out<<",";
                    if (width != 0) out<<std::endl;
                    out<<newTab<<"\""<<element->first<<"\": ";
                    if (element->second->getFlag() < json_value_id::OB_OBJECT) out<<*(element->second);
                    else element->second->print(table + 1,width,out);
                }
                if (width != 0) out<<std::endl;
                out<<tab<<"}";
                return;
            }

            const std::shared_ptr <json_value> & operator [](const std::string & key) const{
                return value.at(key);
            }

            std::shared_ptr <json_value> & operator [](const std::string & key){
                return value[key];
            }

            const std::shared_ptr <json_value> & at(const std::string & key) const{
                return value.at(key);
            }

            std::shared_ptr <json_value> & at(const std::string & key){
                return value.at(key);
            }

            size_t size() const override{
                return value.size();
            }

            void clear() override{
                value.clear();
                return;
            }

            bool equals(const std::shared_ptr <json_value> & other) const override{
                if (this->getFlag() != other->getFlag()) return false;
                else if (this->shared_from_this() == other) return true;
                else{
                    auto o = std::dynamic_pointer_cast <json_object>(other);
                    if (!o) return false;
                    if (this->value.size() != o->value.size()) return false;
                    for(const auto & element : this->value){
                        auto it = o->value.find(element.first);
                        bool exists = it != o->value.end();
                        if (exists) exists = exists && element.second->equals(it->second);
                        if (!exists) return false;
                    }
                    return true;
                }
            }

        };

        class json_array : public json_value{

            public:

            arrayValueType value;

            json_array():json_value(),value(){}
            //json_array(const arrayValueType & v):json_value(),value(v){}
            json_array(arrayValueType && v):json_value(),value(std::move(v)){}
            ~json_array() override = default;

            json_array(const json_array & other):json_value(other){
                for(const auto & element : other.value){
                    value.emplace_back(element->clone());
                }
            }

            json_array & operator =(const json_array & other){
                if (this == &other) return *this;
                json_value::operator =(other);
                for(const auto & element : other.value){
                    value.emplace_back(element->clone());
                }
                return *this;
            }

            std::shared_ptr <json_value> clone() const override{
                return std::make_shared <json_array>(*this);
            }

            json_value_id getFlag() const override{
                return json_value_id::OB_ARRAY;
            }

            inline static json_value_id flag(){
                return json_value_id::OB_ARRAY;
            }

            valueType getValue() const override{
                return value;
            }

            void print(size_t table,size_t width = 4,std::ostream & out = std::cout) const override{
                if (value.empty()){
                    out<<"[]";
                    return;
                }
                std::string tab(table * width,' ');
                std::string newTab((table + 1) * width,' ');
                out<<"[";
                if (width != 0) out<<std::endl;
                auto element = value.begin();
                if (*element != nullptr){
                    out<<newTab;
                    if ((*element)->getFlag() < json_value_id::OB_OBJECT) out<<*(*element);
                    else (*element)->print(table + 1,width,out);
                }
                ++element;
                for(;element != value.end();++element){
                    if (*element == nullptr) continue;
                    out<<",";
                    if (width != 0) out<<std::endl;
                    out<<newTab;
                    if ((*element)->getFlag() < json_value_id::OB_OBJECT) out<<*(*element);
                    else (*element)->print(table + 1,width,out);
                }
                if (width != 0) out<<std::endl;
                out<<tab<<"]";
                return;
            }

            const std::shared_ptr <json_value> & operator [](size_t index) const{
                return value[index];
            }

            std::shared_ptr <json_value> & operator [](size_t index){
                return value[index];
            }

            const std::shared_ptr <json_value> & at(size_t index) const{
                return value.at(index);
            }

            std::shared_ptr <json_value> & at(size_t index){
                return value.at(index);
            }

            size_t size() const override{
                return value.size();
            }

            void clear() override{
                value.clear();
                return;
            }

            bool equals(const std::shared_ptr <json_value> & other) const override{
                if (this->getFlag() != other->getFlag()) return false;
                else if (this->shared_from_this() == other) return true;
                else{
                    auto o = std::dynamic_pointer_cast <json_array>(other);
                    if (!o) return false;
                    if (this->value.size() != o->value.size()) return false;
                    for(size_t i=0;i<this->value.size();++i){
                        if (!this->value[i]->equals(o->value[i])) return false;
                    }
                    return true;
                }
            }


        };

/*
json EBNF文法规则：
json -> object | array
object -> \{ [ key_value { , key_value } ] \}
key_value -> key : value
key -> string
value -> string | number | boolean | null | object | array
boolean -> true | false
array -> \[ [ value { , value } ] \]

non_terminal | first_set                           | follow_set
        json | \{ \[                               | $
      object | \{                                  | $ , \} \]
       array | \[                                  | $ , \} \]
   key_value | string                              | , \}
         key | string                              | :
       value | string number true false null \{ \[ | , \} \]
     boolean | true false                          | , \} \]
*/

        private:

        inline static const std::unordered_map <json_token_id,std::string> errorMessageMap = {
            {json_token_id::LT_STRING,"A string value should be input."},
            {json_token_id::LT_NUMBER,"A json value should be input."},
            {json_token_id::LT_TRUE,"A boolean value should be input."},
            {json_token_id::LT_FALSE,"A boolean value should be input."},
            {json_token_id::LT_NULL,"A json value should be input."},

            {json_token_id::SE_OBJECTLEFT,"A map or array should be input."},
            {json_token_id::SE_OBJECTRIGHT,"A curly brace is not closed."},
            {json_token_id::SE_ARRAYLEFT,"A map or array should be input."},
            {json_token_id::SE_ARRAYRIGHT,"A square bracket is not closed."},
            {json_token_id::SE_COLON,"A colon should be input."},
            {json_token_id::SE_COMMA,"A comma should be input."},

            {json_token_id::TOKEN_ERROR,"An unknown token."}
        };

        json_token parsingTempToken;
        bool parsingNotEnd;

        void outputError(
            json_token_id tokenId,
            std::ostream & err = std::cerr
        ){
            auto it = errorMessageMap.find(tokenId);
            if (it == errorMessageMap.end()) err<<errorMessageMap.at(json_token_id::TOKEN_ERROR)<<std::endl;
            else err<<it->second<<std::endl;
            return;
        }

        void throwError(json_token_id tokenId){
            std::string msg = "Json syntax error : ";
            auto it = errorMessageMap.find(tokenId);
            if (it == errorMessageMap.end()) msg += errorMessageMap.at(json_token_id::TOKEN_ERROR);
            else msg += it->second;
            throw std::runtime_error(msg);
            return;
        }

        void match(
            json_token_id id,
            std::ostream & err = std::cerr,
            const std::string & filePath = "stdin",
            std::istream & in = std::cin
        ){
            if (id == parsingTempToken.tokenId){
                parsingNotEnd = getToken(parsingTempToken,filePath,in);
                return;
            }
            if (id > parsingTempToken.tokenId) parsingNotEnd = getToken(parsingTempToken,filePath,in);
            //outputError(id,err);
            throwError(id);
            return;
        }

        std::shared_ptr <json_boolean> parse_boolean(
            std::ostream & err = std::cerr,
            const std::string & filePath = "stdin",
            std::istream & in = std::cin
        ){
            bool booleanValue = parsingTempToken.getValue <bool>();
            match(parsingTempToken.tokenId,err,filePath,in);
            return std::make_shared <json_boolean>(booleanValue);
        }

        std::shared_ptr <json_value> parse_value(
            std::ostream & err = std::cerr,
            const std::string & filePath = "stdin",
            std::istream & in = std::cin
        ){
            switch (parsingTempToken.tokenId){
                case json_token_id::LT_STRING:{
                    std::string value = parsingTempToken.getValue <std::string>();
                    //std::cout<<"\""<<value<<"\""<<std::endl;
                    match(json_token_id::LT_STRING,err,filePath,in);
                    return std::make_shared <json_string>(value);
                }
                case json_token_id::LT_NUMBER:{
                    long double value = parsingTempToken.getValue <long double>();
                    match(json_token_id::LT_NUMBER,err,filePath,in);
                    return std::make_shared <json_number>(value);
                }
                case json_token_id::LT_TRUE:return parse_boolean(err,filePath,in);
                case json_token_id::LT_FALSE:return parse_boolean(err,filePath,in);
                case json_token_id::LT_NULL:{
                    match(json_token_id::LT_NULL,err,filePath,in);
                    return std::make_shared <json_null>();
                }
                case json_token_id::SE_OBJECTLEFT:return parse_object(err,filePath,in);
                case json_token_id::SE_ARRAYLEFT:return parse_array(err,filePath,in);
                default:{
                    //outputError(json_token_id::LT_NUMBER,err);
                    throwError(json_token_id::LT_NUMBER);
                    return nullptr;
                }
            }
        }

        std::shared_ptr <json_value> parse_pair(
            std::string & key,
            std::ostream & err = std::cerr,
            const std::string & filePath = "stdin",
            std::istream & in = std::cin
        ){
            if (parsingTempToken.tokenId == json_token_id::LT_STRING) key = parsingTempToken.getValue <std::string>();
            //std::cout<<"\""<<key<<"\""<<std::endl;
            match(json_token_id::LT_STRING,err,filePath,in);
            match(json_token_id::SE_COLON,err,filePath,in);
            return parse_value(err,filePath,in);
        }

        std::shared_ptr <json_object> parse_object(
            std::ostream & err = std::cerr,
            const std::string & filePath = "stdin",
            std::istream & in = std::cin
        ){
            match(json_token_id::SE_OBJECTLEFT,err,filePath,in);
            if (parsingTempToken.tokenId == json_token_id::SE_OBJECTRIGHT){
                match(json_token_id::SE_OBJECTRIGHT,err,filePath,in);
                return std::make_shared <json_object>();
            }
            //std::shared_ptr <json_object> o = std::make_shared <json_object>();
            objectValueType v;
            std::string key;
            std::shared_ptr <json_value> value = parse_pair(key,err,filePath,in);
            if (value != nullptr) v.emplace(key,value);
            while (parsingNotEnd && parsingTempToken.tokenId == json_token_id::SE_COMMA){
                match(json_token_id::SE_COMMA,err,filePath,in);
                key.clear();
                value = parse_pair(key,err,filePath,in);
                if (value != nullptr) v.emplace(key,value);
            }
            match(json_token_id::SE_OBJECTRIGHT,err,filePath,in);
            /*for(const auto & element : v){
                if (element.second == nullptr) std::cout<<"\""<<element.first<<"\": nullptr"<<std::endl;
                else std::cout<<"\""<<element.first<<"\": "<<*element.second<<std::endl;
            }*/
            return std::make_shared <json_object>(std::move(v));
        }

        std::shared_ptr <json_array> parse_array(
            std::ostream & err = std::cerr,
            const std::string & filePath = "stdin",
            std::istream & in = std::cin
        ){
            match(json_token_id::SE_ARRAYLEFT,err,filePath,in);
            if (parsingTempToken.tokenId == json_token_id::SE_ARRAYRIGHT){
                match(json_token_id::SE_ARRAYRIGHT,err,filePath,in);
                return std::make_shared <json_array>();
            }
            arrayValueType a;
            std::shared_ptr <json_value> value = parse_value(err,filePath,in);
            if (value != nullptr) a.emplace_back(value);
            while (parsingNotEnd && parsingTempToken.tokenId == json_token_id::SE_COMMA){
                match(json_token_id::SE_COMMA,err,filePath,in);
                value = parse_value(err,filePath,in);
                if (value != nullptr) a.emplace_back(value);
            }
            match(json_token_id::SE_ARRAYRIGHT,err,filePath,in);
            return std::make_shared <json_array>(std::move(a));
        }

        std::shared_ptr <json_value> parse_json(
            std::ostream & err = std::cerr,
            const std::string & filePath = "stdin",
            std::istream & in = std::cin
        ){
            parsingNotEnd = getToken(parsingTempToken,filePath,in);
            if (!parsingNotEnd){
                std::string msg = "zhy::json::parse_json : No token has been input.";
                throw std::runtime_error(msg);
                return nullptr;
            }
            if (parsingTempToken.tokenId == json_token_id::SE_OBJECTLEFT) return parse_object(err,filePath,in);
            else if (parsingTempToken.tokenId == json_token_id::SE_ARRAYLEFT) return parse_array(err,filePath,in);
            else{
                //outputError(json_token_id::SE_OBJECTLEFT,err);
                throwError(json_token_id::SE_OBJECTLEFT);
                return nullptr;
            }
        }

        std::shared_ptr <json_value> jsonValue;

        public:

        json():parsingTempToken(),parsingNotEnd(true),jsonValue(nullptr){}

        json(
            std::istream & in,
            const std::string & filePath = "stdin",
            std::ostream & err = std::cerr
        ):parsingTempToken(),parsingNotEnd(true){
            jsonValue = parse_json(err,filePath,in);
        }

        json(
            const std::string & filePath,
            std::ostream & err = std::cerr
        ):parsingTempToken(),parsingNotEnd(true){
            std::ifstream in(std::filesystem::path(filePath),std::ios::in);
            if (!in.is_open()){
                std::string msg = "zhy::json::constructor : Failed to open file " + filePath + ".";
                throw std::invalid_argument(msg);
            }
            jsonValue = parse_json(err,filePath,in);
            in.close();
        }

        json(const std::shared_ptr <json_value> & jv){
            if (!jv){
                std::string msg = "zhy::json::constructor : Construct a zhy::json from a nullptr.";
                throw std::invalid_argument(msg);
            }
            if (!jv->isJson()){
                std::string msg = "zhy::json::constructor : Construct a zhy::json from a non map or non array.";
                throw std::invalid_argument(msg);
            }
            jsonValue = jv;
        }

        
        ~json() = default;

        json(const zhy::json & other){
            if (!other.jsonValue){
                std::string msg = "zhy::json::constructor : Construct a zhy::json from a nullptr.";
                throw std::invalid_argument(msg);
            }
            jsonValue = other.jsonValue->clone();
        }

        zhy::json & operator =(const zhy::json & other){
            if (!other.jsonValue){
                std::string msg = "zhy::json::operator = : Assign a nullptr to a zhy::json.";
                throw std::invalid_argument(msg);
            }
            if (this != &other) jsonValue = other.jsonValue->clone();
            return *this;
        }

        zhy::json & operator =(const std::shared_ptr <json_value> & jv){
            if (!jv){
                std::string msg = "zhy::json::operator = : Assign a nullptr to a zhy::json.";
                throw std::invalid_argument(msg);
            }
            if (!jv->isJson()){
                std::string msg = "zhy::json::operator = : Assign a non map or non array to a zhy::json.";
                throw std::invalid_argument(msg);
            }
            jsonValue = jv;
            return *this;
        }

        zhy::json share() const{
            return zhy::json(jsonValue);
        }

        zhy::json clone() const{
            return zhy::json(jsonValue->clone());
        }

        json_value_id getFlag() const{
            return jsonValue->getFlag();
        }

        const std::shared_ptr <json_value> & getValuePtr() const{
            return jsonValue;
        }

        std::shared_ptr <json_value> & getValuePtr(){
            return jsonValue;
        }

        const std::shared_ptr <json_value> & operator [](const std::string & key) const{
            if (!jsonValue){
                std::string msg = "zhy::json::operator [] : The value of json is null.";
                throw std::out_of_range(msg);
            }
            if (jsonValue->getFlag() == json_value_id::OB_OBJECT){
                std::shared_ptr <json_object> jsonObject = std::dynamic_pointer_cast <json_object>(jsonValue);
                return jsonObject->operator [](key);
            }
            else{
                std::string msg = "zhy::json::operator [] : Json value is not a map.";
                throw std::out_of_range(msg);
            }
        }

        std::shared_ptr <json_value> & operator [](const std::string & key){
            if (!jsonValue){
                std::string msg = "zhy::json::operator [] : The value of json is null.";
                throw std::out_of_range(msg);
            }
            if (jsonValue->getFlag() == json_value_id::OB_OBJECT){
                std::shared_ptr <json_object> jsonObject = std::dynamic_pointer_cast <json_object>(jsonValue);
                return jsonObject->operator [](key);
            }
            else{
                std::string msg = "zhy::json::operator [] : Json value is not a map.";
                throw std::out_of_range(msg);
            }
        }

        const std::shared_ptr <json_value> & operator [](size_t index) const{
            if (!jsonValue){
                std::string msg = "zhy::json::operator [] : The value of json is null.";
                throw std::out_of_range(msg);
            }
            if (jsonValue->getFlag() == json_value_id::OB_ARRAY){
                std::shared_ptr <json_array> jsonArray = std::dynamic_pointer_cast <json_array>(jsonValue);
                return jsonArray->operator [](index);
            }
            else{
                std::string msg = "zhy::json::operator [] : Json value is not an array.";
                throw std::out_of_range(msg);
            }
        }

        std::shared_ptr <json_value> & operator [](size_t index){
            if (!jsonValue){
                std::string msg = "zhy::json::operator [] : The value of json is null.";
                throw std::out_of_range(msg);
            }
            if (jsonValue->getFlag() == json_value_id::OB_ARRAY){
                std::shared_ptr <json_array> jsonArray = std::dynamic_pointer_cast <json_array>(jsonValue);
                return jsonArray->operator [](index);
            }
            else{
                std::string msg = "zhy::json::operator [] : Json value is not an array.";
                throw std::out_of_range(msg);
            }
        }

        const std::shared_ptr <json_value> & at(const std::string & key) const{
            if (!jsonValue){
                std::string msg = "zhy::json::at : The value of json is null.";
                throw std::out_of_range(msg);
            }
            if (jsonValue->getFlag() == json_value_id::OB_OBJECT){
                std::shared_ptr <json_object> jsonObject = std::dynamic_pointer_cast <json_object>(jsonValue);
                return jsonObject->at(key);
            }
            else{
                std::string msg = "zhy::json::at : Json value is not a map.";
                throw std::out_of_range(msg);
            }
        }

        std::shared_ptr <json_value> & at(const std::string & key){
            if (!jsonValue){
                std::string msg = "zhy::json::at : The value of json is null.";
                throw std::out_of_range(msg);
            }
            if (jsonValue->getFlag() == json_value_id::OB_OBJECT){
                std::shared_ptr <json_object> jsonObject = std::dynamic_pointer_cast <json_object>(jsonValue);
                return jsonObject->at(key);
            }
            else{
                std::string msg = "zhy::json::at : Json value is not a map.";
                throw std::out_of_range(msg);
            }
        }

        const std::shared_ptr <json_value> & at(size_t index) const{
            if (!jsonValue){
                std::string msg = "zhy::json::at : The value of json is null.";
                throw std::out_of_range(msg);
            }
            if (jsonValue->getFlag() == json_value_id::OB_ARRAY){
                std::shared_ptr <json_array> jsonArray = std::dynamic_pointer_cast <json_array>(jsonValue);
                return jsonArray->at(index);
            }
            else{
                std::string msg = "zhy::json::at : Json value is not an array.";
                throw std::out_of_range(msg);
            }
        }

        std::shared_ptr <json_value> & at(size_t index){
            if (!jsonValue){
                std::string msg = "zhy::json::at : The value of json is null.";
                throw std::out_of_range(msg);
            }
            if (jsonValue->getFlag() == json_value_id::OB_ARRAY){
                std::shared_ptr <json_array> jsonArray = std::dynamic_pointer_cast <json_array>(jsonValue);
                return jsonArray->at(index);
            }
            else{
                std::string msg = "zhy::json::at : Json value is not an array.";
                throw std::out_of_range(msg);
            }
        }

        size_t size() const{
            if (!jsonValue){
                std::string msg = "zhy::json::size : The value of json is null.";
                throw std::runtime_error(msg);
            }
            return jsonValue->size();
        }

        void clear(){
            if (!jsonValue){
                std::string msg = "zhy::json::clear : The value of json is null.";
                throw std::runtime_error(msg);
            }
            jsonValue->clear();
        }

        bool equals(const zhy::json & other) const{
            if (this->getFlag() != other.getFlag()) return false;
            else if (this->jsonValue == other.jsonValue) return true;
            else return this->jsonValue->equals(other.jsonValue);
        }

        void print(size_t table = 0,size_t width = 4,std::ostream & out = std::cout) const{
            if (!jsonValue){
                std::string msg = "zhy::json::print : The value of json is null.";
                throw std::runtime_error(msg);
            }
            jsonValue->print(table,width,out);
            return;
        }

        friend std::ostream & operator <<(std::ostream & out,const zhy::json & self){
            if (!self.jsonValue){
                std::string msg = "std::ostream << zhy::json : The value of json is null.";
                throw std::runtime_error(msg);
            }
            out<<*(self.jsonValue);
            return out;
        }

        void display(size_t table,size_t width = 4) const{
            print(table,width,std::cout);
            return;
        }

        void outputFile(const std::string & filePath,size_t table,size_t width = 4) const{
            std::ofstream out(std::filesystem::path(filePath),std::ios::out);
            if (!out.is_open()){
                std::string msg = "Failed to open file " + filePath + ".";
                throw std::invalid_argument(msg);
            }
            print(table,width,out);
            out.close();
            return;
        }

        friend std::istream & operator >>(std::istream & in,zhy::json & self){
            self.jsonValue = self.parse_json(std::cerr,"unknown",in);
            return in;
        }



        inline static void json_tester(){
            std::string testPath = "D:\\project\\zhy_headers\\settings.json";

            /*zhy::json test1;
            std::cin>>test1;
            std::cout<<test1;*/

            zhy::json test2(testPath);
            //std::cout<<test2;

            /*{
                zhy::json test3 = test2;
                zhy::json test4 = test3["files.associations"];
                std::cout<<test4<<std::endl;
                test4.clear();

                zhy::json test5 = test3["C_Cpp_Runner.msvcWarnings"];
                std::cout<<test5<<std::endl;
                test5.clear();

                std::cout<<test3;
            }*/

            {
                zhy::json test3 = test2;
                zhy::json test4 = test3["files.associations"]->clone();
                std::cout<<test4<<std::endl;
                test4.clear();
                std::cout<<test4<<std::endl;

                zhy::json test5 = test3["C_Cpp_Runner.msvcWarnings"]->clone();
                std::cout<<test5<<std::endl;
                test5.clear();
                std::cout<<test5<<std::endl;

                std::cout<<test3;
            }

            return;
        }

    };

    
















}

#endif