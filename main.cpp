

#include "mimalloc-new-delete.h"

#include <iostream>
#include <regex>
#include <map>
#include <unordered_map>
#include "clau_parser.h"
#include "smart_ptr.h"
using namespace std::literals;



enum FUNC { FUNC_GET, FUNC_FIND, FUNC_WHILE, FUNC_RETURN_VALUE, FUNC_IS_END, FUNC_NOT,
	FUNC_LOAD_DATA, FUNC_ENTER, FUNC_CALL, FUNC_NEXT, FUNC_RETURN, FUNC_COMP_RIGHT,
	FUNC_ADD, FUNC_GET_IDX, FUNC_GET_SIZE, FUNC_GET_NOW, FUNC_CLONE, FUNC_QUIT, FUNC_IF, FUNC_IS_ITEM,
	FUNC_IS_GROUP, FUNC_SET_IDX, FUNC_AND, FUNC_IS_QUOTED_STR, FUNC_COMP_LEFT, FUNC_SET_NAME, FUNC_GET_NAME, FUNC_GET_VALUE,
		FUNC_SET_VALUE, FUNC_REMOVE_QUOTED, CONSTANT, THEN, WHILE_END, IF_END, START_DIR, DIR, END_DIR, FUNC_PRINT, NONE,
	KEY, VALUE,  SIZE // chk?
  };
const char* func_to_str[FUNC::SIZE] = {
	"GET",
	"FIND", "WHILE", "RETURN_VALUE", "IS_END", "NOT", "LOAD_DATA", "ENTER", "CALL", "NEXT", "RETURN", "COMP_RIGHT",
	"ADD", "GET_IDX", "GET_SIZE", "GET_NOW", "CLONE", "QUIT", "IF", "IS_ITEM",
	"IS_GROUP", "SET_IDX", "AND", "IS_QUOTED_STR", "COMP_LEFT", "SET_NAME", "GET_NAME", "GET_VALUE",
	"SET_VALUE", "REMOVE_QUOTED", "CONSTANT", "THEN", "WHILE_END", "IF_END", "START_DIR", "DIR", "END_DIR", "PRINT", "NONE", "KEY", "VALUE"
};

class Workspace {
public:
	wiz::SmartPtr<clau_parser::Reader> reader;
public:
	Workspace(clau_parser::UserType* ut = nullptr) {
		if (reader) {
			*reader = clau_parser::Reader(ut);
		}
	}
};

class Token {
public:
	long long int_val = 0;
	long double float_val = 0;
	std::string str_val;
	FUNC func = FUNC::NONE;
	Workspace workspace;
	enum Type { INT = 1, FLOAT = 2, STRING = 4, FUNC = 8, USERTYPE = 16, WORKSPACE = 32, BOOL = 64, NONE = 128 };
	Type type = Type::NONE;
	long long line = 0;

	Token(clau_parser::UserType* ut = nullptr) : workspace(ut) {
		//
	}

	// ToString();
};


static std::vector<Token> FindValue(clau_parser::UserType* ut, const std::string& str)
{ // std::string ´ë½Å vector<std::string> ??
	int count = 0;
	int idx = -1;
	for (int i = str.size() - 1; i >= 0; --i) {
		if ('/' == str[i]) {
			if (count == 0) {
				idx = i;
			}
			count++;
		}
	}

	std::vector<Token> result;

	if (count == 1)
	{ 
		Token token;
		token.type = Token::Type::STRING;
		return { token };
	}
	else {
		auto x = clau_parser::UserType::Find(ut, str.substr(0, idx + 1));

		if (x.first == false) { return result; }

		for (int i = 0; i < x.second.size(); ++i) {
			std::string itemName = str.substr(idx + 1);

			if (itemName._Starts_with("$it") && itemName.size() >= 4) {
				int itemIdx = std::stoi(itemName.substr(3));

				Token temp;
				temp.type = Token::Type::STRING;
				temp.str_val = (x.second[i]->GetItemList(itemIdx).Get(0));
				result.push_back(temp);
			}
			else {
				if (itemName == "_") {
					itemName = "";
				}
				auto temp = x.second[i]->GetItem(itemName);
				if (!temp.empty()) {
					for (int j = 0; j < temp.size(); ++j) {
						Token tkn;
						tkn.type = Token::Type::STRING;
						tkn.str_val = temp[j].Get(0);

						result.push_back(tkn);
					}
				}
			}
		}
	}
	return result;
}


struct Event {
	Workspace workspace;
	std::string id;
	std::vector<int> event_data;
	long long now = 0;
	std::vector<Token> return_value;
	long long return_value_now;
	wiz::SmartPtr<std::vector<Token>> input; // ?
	std::unordered_map<std::string, Token> parameter;
};

	int tokenize_pre(std::string_view sv, char ch) {
		int count = 0;

		size_t x;
		if ((x = sv.find(ch)) == std::string::npos) {
			if (!sv.empty()) {
				count++;
			}
			return count;
		}

		if (x > 0) {
			count++;
		}

		size_t y;
		while (x != std::string::npos) {
			y = sv.find(ch, x + 1);

			if (y == std::string::npos) {
				if (x + 1 < sv.size()) {
					count++;
				}
				break;
			}
			else {
				if (y - 1 - x + 1 > 0) {
					count++;
				}
			}

			x = y;
		}

		return count;
	}


class VM {
private:

	std::pair<bool, std::vector<clau_parser::UserType*> > _Find(clau_parser::UserType* global, const std::string& _position) /// option, option_offset
	{
		std::string position = _position;

		if (!position.empty() && position[0] == '@') { position.erase(position.begin()); }

		std::vector<std::string> x;


		//wiz::Out << "string view is " << pos_sv << " ";
		std::vector<std::string> tokens = clau_parser::tokenize(position, '/');

		for (size_t i = 0; i < tokens.size(); ++i) {
			std::string temp = tokens[i];

			//wiz::Out << tokens[i] << " ";

			if (temp == ".") {
				continue;
			}

			if (x.empty()) {
				x.push_back(temp);
			}
			else if (x.back() != ".." && temp == "..") {
				x.pop_back();
			}
			else {
				x.push_back(temp);
			}
		}

		return __Find(std::move(x), global);
	}
private:
	// find userType! not itemList!,
	// this has bug
	/// /../x ok   ( /../../y ok)
	/// /x/../x/../x no
	std::pair<bool, std::vector< clau_parser::UserType*> > __Find(std::vector<std::string>&& tokens, clau_parser::UserType* global) /// option, option_offset
	{
		std::vector<clau_parser::UserType* > temp;
		int start = 0;

		if (tokens.empty()) { temp.push_back(global); return{ true, temp }; }
		if (tokens.size() == 1 && tokens[0] == ".") { temp.push_back(global); return{ true, temp }; }
		//if (position.size() == 1 && position[0] == "/./") { temp.push_back(global); return{ true, temp }; } // chk..
		//if (position.size() == 1 && position[0] == "/.") { temp.push_back(global); return{ true, temp }; }
		//if (position.size() == 1 && position[0] == "/") { temp.push_back(global); return{ true, temp }; }

		if (tokens.size() > 1 && tokens[0] == ".")
		{
			start = 1;
			//position = String::substring(position, 3);
		}

		std::list<std::pair<clau_parser::UserType*, int >> utDeck;
		std::pair<clau_parser::UserType*, int> utTemp;
		utTemp.first = global;
		utTemp.second = 0;
		std::vector<std::string> strVec;

		//wiz::Out << "position is " << position << "\t";
		for (int i = start; i < tokens.size(); ++i) {
			std::string strTemp = tokens[i];

			//wiz::Out << strTemp << " ";

			if (strTemp == "root" && i == 0) {
			}
			else {
				strVec.push_back(strTemp);
			}

			if ((strVec.size() >= 1) && (" " == strVec[strVec.size() - 1])) /// chk!!
			{
				strVec[strVec.size() - 1] = "";
			}
			else if ((strVec.size() >= 1) && ("_" == strVec[strVec.size() - 1]))
			{
				strVec[strVec.size() - 1] = "";
			}
		}

		// it has bug!
		{
			int count = 0;

			for (int i = 0; i < strVec.size(); ++i) {
				if (strVec[i] == "..") {
					count++;
				}
				else {
					break;
				}
			}

			std::reverse(strVec.begin(), strVec.end());

			for (int i = 0; i < count; ++i) {
				if (utTemp.first == nullptr) {
					return{ false, std::vector< clau_parser::UserType* >() };
				}
				utTemp.first = utTemp.first->GetParent();
				strVec.pop_back();
			}
			std::reverse(strVec.begin(), strVec.end());
		}
		//wiz::Out << "\n";

		utDeck.push_front(utTemp);

		bool exist = false;
		while (false == utDeck.empty()) {
			utTemp = utDeck.front();
			utDeck.pop_front();

			if (utTemp.second < strVec.size() && strVec[utTemp.second] == "$")
			{
				for (int j = utTemp.first->GetUserTypeListSize() - 1; j >= 0; --j) {
					clau_parser::UserType* x = utTemp.first->GetUserTypeList(j);
					utDeck.push_front(std::make_pair(x, utTemp.second + 1));
				}
			}
			else if (utTemp.second < strVec.size() && strVec[utTemp.second]._Starts_with("$.")) /// $."abc"
			{
				std::string rex_str = strVec[utTemp.second].substr(3, strVec[utTemp.second].size() - 4);
				std::regex rgx(rex_str.data());

				for (int j = utTemp.first->GetUserTypeListSize() - 1; j >= 0; --j) {
					if (std::regex_match((utTemp.first->GetUserTypeList(j)->GetName()), rgx)) {
						clau_parser::UserType* x = utTemp.first->GetUserTypeList(j);
						utDeck.push_front(std::make_pair(x, utTemp.second + 1));
					}
				}
			}
			else if (utTemp.second < strVec.size() &&
				(utTemp.first->GetUserTypeItem(strVec[utTemp.second]).empty() == false))
			{
				auto  x = utTemp.first->GetUserTypeItem(strVec[utTemp.second]);
				for (int j = x.size() - 1; j >= 0; --j) {
					utDeck.push_front(std::make_pair(x[j], utTemp.second + 1));
				}
			}

			if (utTemp.second == strVec.size()) {
				exist = true;
				temp.push_back(utTemp.first);
			}
		}
		if (false == exist) {
			return{ false, std::vector<clau_parser::UserType*>() };
		}
		return{ true, temp };
	}

	std::vector<Token> Find(clau_parser::UserType* ut, std::string str) {
		std::vector<Token> result;
		auto uts = _Find(ut, str);
		
		if (!uts.first) {
			return result;
		}

		for (long long i = 0; i < uts.second.size(); ++i) {
			Token _token;
			_token.type = Token::Type::WORKSPACE;
			_token.workspace.reader = new clau_parser::Reader(uts.second[i]);
			result.push_back(_token);
		}
		
		return result;
	}
public:

	
	void Run(const std::string& id, clau_parser::UserType* global) {
		Event main = _event_list[id];
		std::vector<Token> token_stack;

		_stack.push_back(main);
		int count = 0;
		std::string dir = "";


		while (!_stack.empty()) {
			auto& x = _stack.back();
			count++;

	//std::cout << func_to_str[x.event_data[x.now]] << "\n";

			switch (x.event_data[x.now]) {
			case FUNC::FUNC_GET:
			{
				auto token = token_stack.back();
				token_stack.pop_back();

				/// <summary>
				/// Todo - from clauscript 
				/// </summary>
				/// <param name="id"></param>
				/// <param name="global"></param>
				auto value = FindValue(global, token.str_val); // ToString?

				token_stack.push_back(value[0]);
			}
			break;
			case FUNC::START_DIR:
				count = 0;
				dir = "/";
				break;
			case FUNC::DIR:
				////std::cout << "DIR chk" << token_stack.back().str_val << "\n";
			{
				auto str = token_stack.back().str_val;

				if (str._Starts_with("$parameter.")) {
					str = str.substr(11);

					Token token = x.parameter[str];
					dir += token.str_val; // ToString.
				}
				else {
					dir += token_stack.back().str_val; // ToString
				}

				token_stack.pop_back();
				dir += "/";
			}

			break;
			case FUNC::END_DIR:
			{
				Token token;
				token.type = Token::STRING;
				if (dir.back() == '/') {
					token.str_val = dir.substr(0, dir.size() - 1);
				}
				else {
					token.str_val = dir;
				}
				token_stack.push_back(token);

				dir = "";
				count = 0;
			}
			break;
			case FUNC::FUNC_REMOVE_QUOTED:
			{
				auto str = token_stack.back().str_val;

				if (str.size() >= 2) {
					str = str.substr(1, str.size() - 2);
				}
				token_stack.pop_back();

				Token temp;
				temp.str_val = str;
				temp.type = Token::Type::STRING;
				token_stack.push_back(temp);
			}
				break;
			case FUNC::FUNC_IS_QUOTED_STR:
			{
				auto str = token_stack.back().str_val;
				bool chk = str.size() >= 2 && str[0] == str.back() && str.back() == '\"';

				token_stack.pop_back();

				Token temp;
				temp.int_val = chk ? 1 : 0;
				temp.type = Token::Type::BOOL;
				token_stack.push_back(temp);
				break;
			}
			case FUNC::FUNC_RETURN:
				//std::cout << "return .... \n";)

				_stack.pop_back();
				break;
			case FUNC::CONSTANT:
				x.now++;

				{
					const auto& value = (*x.input)[x.event_data[x.now]];

					if (value.type == Token::Type::STRING) {
						if (value.str_val._Starts_with("$parameter.")) {
							auto param = value.str_val.substr(11);

							token_stack.push_back(x.parameter[param]);

							x.now++;
							continue;
						}
					}

					{
						token_stack.push_back(value);
					}
				}

				break;
			case FUNC::FUNC_ADD:
			{
				long long a = std::stoll(token_stack.back().str_val);
				token_stack.pop_back();
				long long b = std::stoll(token_stack.back().str_val);
				token_stack.pop_back();
				{
					Token token;
					token.str_val = std::to_string(a + b);
					token.type = Token::Type::STRING;
					token_stack.push_back(token);
				}
			}
			break;
			case FUNC::FUNC_CALL:
				x.now++;

				{
					auto count = x.event_data[x.now];

					x.now++;

					Event e;

					while (!(token_stack.back().type == Token::Type::FUNC && token_stack.back().func == FUNC::FUNC_CALL)) { 
						auto value = token_stack.back();
						token_stack.pop_back();
						auto name = token_stack.back();
						token_stack.pop_back();

						if (name.str_val == "id"sv) {
							e.id = value.str_val; 

					//		//std::cout << e.id << "\n";

							e.event_data = _event_list[value.str_val].event_data;
							e.input = _event_list[value.str_val].input;
							e.now = 0;
							e.return_value_now = 0;
							
							break;
						}
						else if (value.type == Token::Type::STRING) {
							if (value.str_val._Starts_with("/$parameter.")) {
								value = x.parameter[value.str_val.substr(11)];

								if (value.str_val._Starts_with("$return_value")) {
									value = x.return_value[x.return_value_now];
								}
							}
						}

						e.parameter[name.str_val] = std::move(value); // name.ToString()
					}

					////std::cout << "call " << e.id << "\n";
					_stack.push_back(std::move(e));
				}

				continue;

				break;
			case FUNC::FUNC_COMP_LEFT:
				// Compare!
			{
				auto b = token_stack.back().str_val;
				token_stack.pop_back();
				auto a =  token_stack.back().str_val;
				token_stack.pop_back();

				{
					Token token;
					token.int_val = a > b;
					token.type = Token::Type::BOOL;
					token_stack.push_back(token);
				}
			}
			break;
			case FUNC::FUNC_COMP_RIGHT:
			{
				long long b = std::stoll(token_stack.back().str_val);
				token_stack.pop_back();
				long long a = std::stoll(token_stack.back().str_val);
				token_stack.pop_back();

				{
					Token token;
					token.int_val = a < b;
					token.type = Token::Type::BOOL;
					token_stack.push_back(token);
				}
			}
			break;
			case FUNC::FUNC_FIND:
			{
				auto a = token_stack.back();
				token_stack.pop_back();

				x.return_value = Find(global, a.str_val);
				x.return_value_now = 0;
			}
			break;
			case FUNC::FUNC_RETURN_VALUE:
			{
				token_stack.push_back(x.return_value[x.return_value_now]);
			}
			break;
			case FUNC::FUNC_NEXT:
				x.return_value_now++;
				break;
			case FUNC::FUNC_LOAD_DATA:
			{
				std::string fileName = token_stack.back().str_val;
				fileName = fileName.substr(1, fileName.size() - 2);
				token_stack.pop_back();


				clau_parser::UserType* dir = token_stack.back().workspace.reader->GetUT();
				token_stack.pop_back();

				// fileName = substr... - todo!
				clau_parser::LoadData::LoadDataFromFile(fileName, *dir, 0, 0);
			}
			break;

			case FUNC::FUNC_ENTER:
				token_stack.back().workspace.reader->Enter();
				token_stack.pop_back();
				break;
			case FUNC::FUNC_QUIT:
				token_stack.back().workspace.reader->Quit();
				token_stack.pop_back();
				break;
			case FUNC::FUNC_SET_NAME:
			{
				auto name = token_stack.back().str_val;
				
				token_stack.pop_back();

				token_stack.back().workspace.reader->SetKey(name);
				
				token_stack.pop_back();
			}
			break;
			case FUNC::FUNC_SET_VALUE:
			{
				auto value = token_stack.back().str_val;
				
				token_stack.pop_back();

				token_stack.back().workspace.reader->SetData(value);
				
				token_stack.pop_back();
			}
			break;
			case FUNC::FUNC_GET_NAME:
			{
				Token token;
				token.type = Token::Type::STRING;
				token.str_val = token_stack.back().workspace.reader->GetKey();

				token_stack.pop_back();
				token_stack.push_back(token);
			}
			break;
			case FUNC::FUNC_GET_VALUE:
			{
				Token token;
				token.type = Token::Type::STRING;
				token.str_val = token_stack.back().workspace.reader->GetData();

				token_stack.pop_back();
				token_stack.push_back(token);
			}
			break;
			case FUNC::FUNC_GET_IDX:
			{
				Token token;
				token.type = Token::Type::STRING;
				token.str_val = std::to_string(token_stack.back().workspace.reader->GetIdx());

				token_stack.pop_back();
				token_stack.push_back(token);
			}
			break;

			case FUNC::FUNC_SET_IDX:
			{
				//auto a = std::stoll(x.input[x.event_data[x.now]].str_val);

				auto a = std::stoll(token_stack.back().str_val);
				token_stack.pop_back();

				auto space = token_stack.back().workspace;
				token_stack.pop_back();

				space.reader->SetIndex(a);
			}
			break;

			case FUNC::FUNC_WHILE:
			{
				//std::cout << "WHILE.... \n";
			}
			break;

			case FUNC::FUNC_AND: // todo 
			{
				x.now++;

				bool result = true;

				for (int i = 0; i < 2; i += 1) {
					bool b = token_stack.back().int_val;
					result = result && b;
					token_stack.pop_back();
				}

				Token temp;
				temp.int_val = result ? 1 : 0;
				temp.type = Token::Type::BOOL;

				token_stack.push_back(temp);
			}
				break;
			case FUNC::FUNC_GET_NOW:
			{
				auto space = token_stack.back().workspace;
				token_stack.pop_back();

				Token temp;
				temp.workspace = space.reader->GetUT();
				temp.type = Token::Type::WORKSPACE;

				token_stack.push_back(temp);

			}
				break;

			case FUNC::THEN:
			{
				auto param = token_stack.back().int_val; // bool
				token_stack.pop_back();

				if (param) {
					x.now++;
				}
				else {
					x.now++;

					x.now = x.event_data[x.now];

					x.now--;
				}
			}
			break;
			case FUNC::WHILE_END:
			{
				x.now++;
				x.now = x.event_data[x.now];

				//std::cout << "chk .. " << func_to_str[x.event_data[x.now]] << "\n";

				x.now--;
			}
			break;
			case FUNC::FUNC_IF:
			{
				//
			}
			break;

			case FUNC::IF_END:
			{
				//
			}
			break;
			case FUNC::FUNC_IS_END:
			{
				Token token;

				token.type = Token::Type::BOOL;
				token.int_val = x.return_value_now >= x.return_value.size();

				token_stack.push_back(token);
			}
			break;
			case FUNC::FUNC_NOT:
			{
				auto a = token_stack.back();
				token_stack.pop_back();

				a.int_val = a.int_val? 0 : 1;
				a.type = Token::Type::BOOL;
				token_stack.push_back(a);
			}
			break;
			case FUNC::FUNC_IS_GROUP:
			{
				Token token;
				token.type = Token::Type::BOOL;

				token.int_val = token_stack.back().workspace.reader->IsGroup()? 1 : 0;
				token_stack.pop_back();

				token_stack.push_back(token);
			}
			break;
			case FUNC::FUNC_IS_ITEM:
			{
				Token token;
				token.type = Token::Type::BOOL;

				token.int_val = !token_stack.back().workspace.reader->IsGroup();
				token_stack.pop_back();

				token_stack.push_back(token);
			}
			break;
			case FUNC::FUNC_GET_SIZE:
			{
				Token token;
				token.type = Token::Type::STRING;

				token.str_val = std::to_string(token_stack.back().workspace.reader->Length());
				token_stack.pop_back();

				token_stack.push_back(token);

				break;
			}
			case FUNC::FUNC_CLONE:
			{
				auto a = token_stack.back().workspace;
				token_stack.pop_back();

				Token b;
				b.workspace.reader = wiz::SmartPtr<clau_parser::Reader>(new clau_parser::Reader(*a.reader));
				token_stack.push_back(b);
			}
				break;

			case FUNC::FUNC_PRINT:
				if (token_stack.back().str_val == "\\n") {
					std::cout << "\n";
				}
				else {
					std::cout << token_stack.back().str_val;
				}
				
				token_stack.pop_back();
				break;
			default:
				//std::cout << "error \n";
				break;;

			}
			x.now++;
		}
	}

	void Register(Event e) {
		_event_list.insert(std::make_pair(e.id, e));
	}

private:
	std::unordered_map<std::string, Event> _event_list;
	std::vector<Event> _stack;
};


int _MakeByteCode(clau_parser::UserType* ut, Event* e) {
	int len = 0;
	long long it_count = 0, ut_count = 0;

	for (long long i = 0; i < ut->GetIListSize(); ++i) {
		if (ut->IsItemList(i)) {
			if (!ut->GetItemList(it_count).GetName().empty()) {
				if (ut->GetItemList(it_count).GetName() == "id"sv && ut->GetName() == "Event"sv) {
					it_count++;
					continue;
				}

				Token token(ut);
				token.type = Token::Type::STRING;
				token.str_val = ut->GetItemList(it_count).GetName();

				e->input->push_back(token);
				e->event_data.push_back(FUNC::CONSTANT);
				e->event_data.push_back(e->input->size() - 1);

				len++;
				len++;

				{
					auto a = ut->GetItemList(it_count).Get();

					if (a._Starts_with("/")) {

						e->event_data.push_back(FUNC::START_DIR);
						len++;
						auto tokens = clau_parser::tokenize(a, '/');

						for (int i = 0; i < tokens.size(); ++i) {
							if (tokens[i]._Starts_with("$"sv) && !tokens[i]._Starts_with("$parameter."sv)) {
								clau_parser::UserType new_ut;

								new_ut.AddUserTypeItem(clau_parser::UserType(tokens[i]));

								len += _MakeByteCode(&new_ut, e);
							}
							else {
								Token token(ut);
								token.type = Token::Type::STRING;
								token.str_val = tokens[i];

								e->input->push_back(token);
								e->event_data.push_back(FUNC::CONSTANT); len++;
								e->event_data.push_back(e->input->size() - 1); len++;
								e->event_data.push_back(FUNC::DIR); len++;
							}
						}

						e->event_data.push_back(FUNC::END_DIR); len++;
					}
					else if (a._Starts_with("@")) {
						auto tokens = clau_parser::tokenize(a, '@');

						for (int i = 0; i < tokens.size(); ++i) {
							if (tokens[i]._Starts_with("$"sv) && !tokens[i]._Starts_with("$parameter."sv)) {
								clau_parser::UserType new_ut;

								new_ut.AddUserTypeItem(clau_parser::UserType(tokens[i]));

								len += _MakeByteCode(&new_ut, e);
								break;
							}
							else {
								Token token(ut);
								token.type = Token::Type::STRING;
								token.str_val = tokens[i];

								e->input->push_back(token);
								e->event_data.push_back(FUNC::CONSTANT); len++;
								e->event_data.push_back(e->input->size() - 1); len++;
							}
						}
					}
					else {
						Token token;
						token.type = Token::Type::STRING;
						token.str_val = ut->GetItemList(it_count).Get();

						e->input->push_back(token);

						e->event_data.push_back(FUNC::CONSTANT); len++;
						e->event_data.push_back(e->input->size() - 1); len++;
					}
				}
			}

			// $while, $if
			else if (ut->GetItemList(it_count).GetName().empty()) {
				if (ut->GetItemList(it_count).Get() == "$while"sv) {
					e->event_data.push_back(FUNC::FUNC_WHILE); len++;
					int idx = e->event_data.size() - 1;
					int then_idx = 0;
					int count1 = _MakeByteCode(ut->GetUserTypeList(ut_count), e);
					len += count1;

					ut_count++; ++i;

					e->event_data.push_back(FUNC::THEN); len++;
					e->event_data.push_back(0); len++;
					then_idx = e->event_data.size() - 1;
					{
						//Event _e;
						//int count2 = _MakeByteCode(ut->GetUserTypeList(ut_count), &_e);
						//len += count2;
						//e->event_data.push_back(0); len++;
					}


					int count2 = _MakeByteCode(ut->GetUserTypeList(ut_count), e);
					len += count2;
					ut_count++; ++i;

					e->event_data.push_back(FUNC::WHILE_END); len++;
					e->event_data.push_back(idx); len++;

					e->event_data[then_idx] = e->event_data.size(); len++;
				}
				else if (ut->GetItemList(it_count).Get() == "$if"sv) {
					e->event_data.push_back(FUNC::FUNC_IF); len++;
					int idx = 0;

					len += _MakeByteCode(ut->GetUserTypeList(ut_count), e);
					ut_count++; ++i;

					e->event_data.push_back(FUNC::THEN); len++;
					e->event_data.push_back(0); len++;
					idx = e->event_data.size() - 1;

					len += _MakeByteCode(ut->GetUserTypeList(ut_count), e);

					ut_count++; ++i;

					e->event_data.push_back(FUNC::IF_END);
					e->event_data[idx] = e->event_data.size(); len++;
				}
				else  {
					auto a = ut->GetItemList(it_count).Get();

					if (a._Starts_with("/")) {

						e->event_data.push_back(FUNC::START_DIR);
						len++;
						auto tokens = clau_parser::tokenize(a, '/');

						for (int i = 0; i < tokens.size(); ++i) {
							if (tokens[i]._Starts_with("$"sv) && !tokens[i]._Starts_with("$parameter."sv)) {
								clau_parser::UserType new_ut;

								new_ut.AddUserTypeItem(clau_parser::UserType(tokens[i]));

								len += _MakeByteCode(&new_ut, e);
								break;
							}
							else {
								Token token(ut);
								token.type = Token::Type::STRING;
								token.str_val = tokens[i];

								e->input->push_back(token);
								e->event_data.push_back(FUNC::CONSTANT); len++;
								e->event_data.push_back(e->input->size() - 1); len++;
								e->event_data.push_back(FUNC::DIR); len++;
							}
						}

						e->event_data.push_back(FUNC::END_DIR); len++;
					}
					else if (a._Starts_with("@")) {
						auto tokens = clau_parser::tokenize(a, '@');

						for (int i = 0; i < tokens.size(); ++i) {
							if (tokens[i]._Starts_with("$"sv) && !tokens[i]._Starts_with("$parameter."sv)) {
								clau_parser::UserType new_ut;

								new_ut.AddUserTypeItem(clau_parser::UserType(tokens[i]));

								len += _MakeByteCode(&new_ut, e);
							}
							else {
								Token token(ut);
								token.type = Token::Type::STRING;
								token.str_val = tokens[i];

								e->input->push_back(token);
								e->event_data.push_back(FUNC::CONSTANT); len++;
								e->event_data.push_back(e->input->size() - 1); len++;
							}
						}
					}
					else {
						Token token;
						token.type = Token::Type::STRING;
						token.str_val = ut->GetItemList(it_count).Get();

						e->input->push_back(token);

						e->event_data.push_back(FUNC::CONSTANT); len++;
						e->event_data.push_back(e->input->size() - 1); len++;
					}
				}
			}
			
			it_count++;
		}
		else {
			std::string name = ut->GetUserTypeList(ut_count)->GetName();
			bool call_flag = false;

			if (name == "$call"sv) {
				call_flag = true;
			}

			int count = _MakeByteCode(ut->GetUserTypeList(ut_count), e);
			len += count;

			if (!ut->GetUserTypeList(ut_count)->GetName().empty()) {
				if (name._Starts_with("$")) {
					Token token(ut);

					token.type = Token::Type::FUNC; // | Token::Type::UserType
					if (name == "$clone"sv) {
						token.func = FUNC::FUNC_CLONE;

						e->event_data.push_back(FUNC::FUNC_CLONE); len++;
					}
					else if (name == "$call"sv) {
						token.func = FUNC::FUNC_CALL;

						e->event_data.push_back(FUNC::FUNC_CALL); len++;
						e->event_data.push_back(count); len++;

					}
					else if (name == "$set_idx"sv) {
						token.func = FUNC::FUNC_SET_IDX;
						
						e->event_data.push_back(FUNC::FUNC_SET_IDX); len++;
					}
					else if (name == "$add"sv) {
						token.func = FUNC::FUNC_ADD;

						e->event_data.push_back(FUNC::FUNC_ADD); len++;
					}
					else if (name == "$get"sv) {
						token.func = FUNC::FUNC_GET;
						
						e->event_data.push_back(FUNC::FUNC_GET); len++;
					}
					else if (name == "$get_name"sv) {
						token.func = FUNC::FUNC_GET_NAME;

						e->event_data.push_back(FUNC::FUNC_GET_NAME); len++;
					}
					else if (name == "$find"sv) {
						token.func = FUNC::FUNC_FIND;

						e->event_data.push_back(FUNC::FUNC_FIND); len++;
					}
					else if (name == "$NOT"sv) {
						token.func = FUNC::FUNC_NOT;

						e->event_data.push_back(FUNC::FUNC_NOT); len++;
					}
					else if (name == "$is_end"sv) {
						token.func = FUNC::FUNC_IS_END;

						e->event_data.push_back(FUNC::FUNC_IS_END); len++;
					}
					else if (name == "$load_data"sv) {
						token.func = FUNC::FUNC_LOAD_DATA;

						e->event_data.push_back(FUNC::FUNC_LOAD_DATA); len++;
					}
					else if (name == "$is_end"sv) {
						token.func = FUNC::FUNC_IS_END;

						e->event_data.push_back(FUNC::FUNC_IS_END); len++;
					}
					else if (name == "$next"sv) {
						token.func = FUNC::FUNC_NEXT;

						e->event_data.push_back(FUNC::FUNC_NEXT); len++;
					}
					else if (name == "$enter") {
						token.func = FUNC::FUNC_ENTER;

						e->event_data.push_back(FUNC::FUNC_ENTER); len++;
					}
					else if (name == "$quit") {
						token.func = FUNC::FUNC_QUIT;

						e->event_data.push_back(FUNC::FUNC_QUIT); len++;
					}
					else if (name == "$parameter"sv) {
						for (int i = 0; i < count; i += 2) {
							
							auto name = (*e->input)[e->event_data.back()].str_val;
							e->event_data.pop_back();
							e->event_data.pop_back();

							e->parameter[name] = Token();
						}
					}
					else if (name == "$COMP<"sv) {
						token.func = FUNC::FUNC_COMP_RIGHT;

						e->event_data.push_back(FUNC::FUNC_COMP_RIGHT); len++;
					}
					else if (name == "$COMP>"sv) {
						token.func = FUNC::FUNC_COMP_LEFT;

						e->event_data.push_back(FUNC::FUNC_COMP_LEFT); len++;
					}
					else if (name == "$AND") {
						token.func = FUNC::FUNC_AND;

						e->event_data.push_back(FUNC::FUNC_AND); len++;
						e->event_data.push_back(count); len++;
					}
					else if (name == "$get_size"sv) {
						token.func = FUNC::FUNC_GET_SIZE;

						e->event_data.push_back(FUNC::FUNC_GET_SIZE); len++;
					}
					else if (name == "$get_idx"sv) {
						token.func = FUNC::FUNC_GET_IDX;

						e->event_data.push_back(FUNC::FUNC_GET_IDX); len++;
					}
					else if (name == "$return_value"sv) {
						token.func = FUNC::FUNC_RETURN_VALUE;

						e->event_data.push_back(FUNC::FUNC_RETURN_VALUE); len++;
					}
					else if (name == "$set_name"sv) {
						token.func = FUNC::FUNC_SET_NAME;

						e->event_data.push_back(FUNC::FUNC_SET_NAME); len++;
					}
					else if (name == "$get_value") {
						token.func = FUNC::FUNC_GET_VALUE;

						e->event_data.push_back(FUNC::FUNC_GET_VALUE); len++;
					}
					else if (name == "$set_value"sv) {
						token.func = FUNC::FUNC_SET_VALUE;

						e->event_data.push_back(FUNC::FUNC_SET_VALUE); len++;
					}
					else if (name == "$is_item"sv) {
						token.func = FUNC::FUNC_IS_ITEM;

						e->event_data.push_back(FUNC::FUNC_IS_ITEM); len++;
					}
					else if (name == "$is_group"sv) {
						token.func = FUNC::FUNC_IS_GROUP;

						e->event_data.push_back(FUNC::FUNC_IS_GROUP); len++;
					}
					else if (name == "$is_quoted_str"sv) {
						token.func = FUNC::FUNC_IS_QUOTED_STR;

						e->event_data.push_back(FUNC::FUNC_IS_QUOTED_STR); len++;
					}
					else if (name == "$remove_quoted"sv) {
						token.func = FUNC::FUNC_REMOVE_QUOTED;

						e->event_data.push_back(FUNC::FUNC_REMOVE_QUOTED); len++;
					}
					else if (name == "$get_now"sv) {

						token.func = FUNC::FUNC_GET_NOW;

						e->event_data.push_back(FUNC::FUNC_GET_NOW); len++;
					}
					else if (name == "$print"sv) {	
						token.func = FUNC::FUNC_PRINT;

						e->event_data.push_back(FUNC::FUNC_PRINT); len++;
					}

					// todo - add processing. errors..

					e->input->push_back(token);
				}
				else {
					Token token(ut);
					token.type = static_cast<Token::Type>(Token::Type::STRING | Token::Type::USERTYPE);
					token.str_val = std::move(name);
					token.type = Token::Type::STRING;

					e->input->push_back(token);
					e->event_data.push_back(FUNC::CONSTANT); len++;
					e->event_data.push_back(e->input->size() - 1); len++;
				}
			}

			ut_count++;
		}
	}
	return len;
}

void Debug(const Event& e) {
	for (int i = 0; i < e.event_data.size(); ++i) {
		if (e.event_data[i] < FUNC::SIZE) {
			//std::cout << func_to_str[e.event_data[i]] << " ";
		}
		else {
			//std::cout << e.event_data[i] << " \n";
		}
	}
}
// need to exception processing.
Event MakeByteCode(clau_parser::UserType* ut) {
	Event e;
	e.input = wiz::SmartPtr<std::vector<Token>>(new std::vector<Token>());

	_MakeByteCode(ut, &e);

	
	e.event_data.push_back(FUNC::FUNC_RETURN);
	e.id = ut->GetItem("id")[0].Get();

	Debug(e);

	return e;
}


int main(void)
{
	VM vm;
	int start = clock();
	clau_parser::UserType global;
	clau_parser::LoadData::LoadDataFromFile("C:\\Users\\vztpv\\source\\repos\\ClauScriptPlusPlus\\ClauScriptPlusPlus\\test.txt",
		global, 1, 0); // DO NOT change!

	auto arr = global.GetUserTypeIdx("Event");
	

	for (auto x : arr) {
		//std::cout << x << " ";
		//std::cout << global.GetUserTypeList(x)->ToString() << "\n";
		vm.Register(MakeByteCode(global.GetUserTypeList(x)));
	}
	for (int i = arr.size() - 1; i >= 0; --i) {
		global.RemoveUserTypeList(arr[i]);
	}

	vm.Run("main", &global);

	int last = clock();
		std::cout << last - start << "ms\n";
	
	clau_parser::LoadData::Save(global, "output.eu4");

	return 0;
}

