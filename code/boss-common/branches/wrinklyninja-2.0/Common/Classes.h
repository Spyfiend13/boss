/*	Better Oblivion Sorting Software
	
	A "one-click" program for users that quickly optimises and avoids 
	detrimental conflicts in their TES IV: Oblivion, Nehrim - At Fate's Edge, 
	TES V: Skyrim, Fallout 3 and Fallout: New Vegas mod load orders.

    Copyright (C) 2009-2012    BOSS Development Team.

	This file is part of Better Oblivion Sorting Software.

    Better Oblivion Sorting Software is free software: you can redistribute 
	it and/or modify it under the terms of the GNU General Public License 
	as published by the Free Software Foundation, either version 3 of 
	the License, or (at your option) any later version.

    Better Oblivion Sorting Software is distributed in the hope that it will 
	be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Better Oblivion Sorting Software.  If not, see 
	<http://www.gnu.org/licenses/>.

	$Revision: 3135 $, $Date: 2011-08-17 22:01:17 +0100 (Wed, 17 Aug 2011) $
*/

#ifndef __BOSS_CLASSES__
#define __BOSS_CLASSES__

#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/fusion/adapted/struct/detail/extension.hpp>
#include "Common/DllDef.h"
#include "Common/Globals.h"
#include "Common/Error.h"

namespace boss {
	using namespace std;
	namespace fs = boost::filesystem;

	enum keyType : uint32_t {
		NONE,
		//RuleList keywords.
		ADD,
		OVERRIDE,
		FOR,
		BEFORE,
		AFTER,
		TOP,
		BOTTOM,
		APPEND,
		REPLACE,
		//Masterlist keywords.
		SAY,
		TAG,
		REQ,
		INC,
		DIRTY,
		WARN,
		ERR,
	};

	enum itemType : uint32_t {
		MOD,
		BEGINGROUP,
		ENDGROUP,
		REGEX
	};

	//////////////////////////////
	// Masterlist Classes
	//////////////////////////////

	//Base class for all conditional data types.
	class conditionalData {	
		friend struct boost::fusion::extension::access;
	private:
		string data;
		string conditions;
	public:
		conditionalData();
		conditionalData(string inData, string inConditions);

		string Data() const;
		string Conditions() const;
		void Data(string inData);
		void Conditions(string inConditions);

		bool evalConditions(boost::unordered_set<string> setVars, boost::unordered_map<string,uint32_t> fileCRCs, ParsingError& errorBuffer);
	};

	class MasterlistVar : public conditionalData {
		friend struct boost::fusion::extension::access;
	public:
		MasterlistVar();
		MasterlistVar(string inData, string inConditions);
	};

	class Message : public conditionalData {
		friend struct boost::fusion::extension::access;
	private:
		keyType key;
	public:
		Message();
		Message(keyType inKey, string inData);

		keyType Key() const;
		void Key(keyType inKey);
		string KeyToString() const;		//Has HTML-safe output.

		bool evalConditions(boost::unordered_set<string> setVars, boost::unordered_map<string,uint32_t> fileCRCs, ParsingError& errorBuffer);
	};

	class Item : public conditionalData {
		friend struct boost::fusion::extension::access;
	private:
		vector<Message> messages;
		//string data is now filename (or group name). Trimmed and case-preserved. ".ghost" extensions are removed.
		itemType		type;
	public:

		Item		();
		Item		(string inName);
		Item		(string inName, itemType inType);
		Item		(string inName, itemType inType, vector<Message> inMessages);

		vector<Message> Messages() const;
		itemType Type() const;
		string Name() const;
		void Messages(vector<Message> inMessages);
		void Type(itemType inType);
		void Name(string inName);

		bool	operator<	(Item);		//Throws boss_error exception on fail.
		
		bool	IsPlugin	();
		bool	IsGroup		();
		bool	IsMasterFile();
		bool	IsGhosted	();			//Checks if the file exists in ghosted form.
		bool	Exists		();			//Checks if the file exists in data_path, ghosted or not.
		string	GetVersion	();			//Outputs the file's header.
		void	SetModTime	(time_t modificationTime);

		bool evalConditions(boost::unordered_set<string> setVars, boost::unordered_map<string,uint32_t> fileCRCs, ParsingError& errorBuffer);
	};

	class ItemList {
	private:
		vector<Item>			items;
		ParsingError			errorBuffer;
		vector<Message>			globalMessageBuffer;
		size_t					lastRecognisedPos;
		vector<MasterlistVar>	masterlistVariables;
		boost::unordered_map<string,uint32_t> fileCRCs;
	public:

								ItemList();
		void					Load			(fs::path path);	//Load by scanning path. If path is a directory, it scans it for plugins. 
																	//If path is a file, it parses it using the modlist grammar.
																	//May throw exception on fail.
		void					Save			(fs::path file);	//Output to file in MF2. Backs up any existing file with new ".old" extension.
																	//Throws exception on fail.
		void					evalConditions();					//Evaluates the conditionals for each item, discarding those items whose conditionals evaluate to false. Also evaluates global message conditionals.
		size_t					FindItem		(string name);	//Find the position of the item with name 'name'. Case-insensitive.
		size_t					FindLastItem	(string name);	//Find the last item with the name 'name'. Case-insensitive.
		size_t					FindGroupEnd	(string name);	//Find the end position of the group with the given name. Case-insensitive.

		vector<Item>							Items() const;
		ParsingError							ErrorBuffer() const;
		vector<Message>							GlobalMessageBuffer() const;
		size_t									LastRecognisedPos() const;
		vector<MasterlistVar>					Variables() const;
		boost::unordered_map<string,uint32_t>	FileCRCs() const;
		void	Items(vector<Item> items);
		void	ErrorBuffer(ParsingError buffer);
		void	GlobalMessageBuffer(vector<Message> buffer);
		void	LastRecognisedPos(size_t pos);
		void	Variables(vector<MasterlistVar> variables);
		void	FileCRCs(boost::unordered_map<string,uint32_t> crcs);

		void Erase(size_t pos);
		void Erase(size_t startPos, size_t endPos);
		void Insert(size_t pos, vector<Item> source, size_t sourceStart, size_t sourceEnd);
		void Insert(size_t pos, Item item);
	};

	//////////////////////////////
	// Userlist Classes
	//////////////////////////////
	
	class RuleLine {
		friend struct boost::fusion::extension::access;
	private:
		keyType key;
		string	object;
	public:
				RuleLine			();
				RuleLine			(keyType inKey, string inObject);

		bool	IsObjectMessage		();
		keyType ObjectMessageKey	();
		string	ObjectMessageData	();
		string	KeyToString			() const;		//Has HTML-safe output.

		keyType Key() const;
		string Object() const;
		void Key(keyType inKey);
		void Object(string inObject);
	};

	class Rule : public RuleLine {
		friend struct boost::fusion::extension::access;
	private:
		bool				enabled;
		vector<RuleLine>	lines;
	public:
		Rule();
		bool Enabled() const;
		vector<RuleLine> Lines() const;

		void Enabled(bool e);
		void Lines(vector<RuleLine> inLines);
	};

	class RuleList {
	public:
		vector<Rule>			rules;
		ParsingError			parsingErrorBuffer;
		vector<ParsingError>	syntaxErrorBuffer;

		RuleList();
		void 					Load	(fs::path file);		//Throws exception on fail.
		void 					Save	(fs::path file);		//Throws exception on fail.
		size_t				 	FindRule(string ruleObject, bool onlyEnabled);

		vector<Rule> Rules();
		ParsingError ParsingErrorBuffer();
		vector<ParsingError> SyntaxErrorBuffer();
		
		void Rules(vector<Rule> inRules);
		void ParsingErrorBuffer(ParsingError buffer);
		void SyntaxErrorBuffer(vector<ParsingError> buffer);
	};

	//////////////////////////////
	// Other Classes
	//////////////////////////////

	class Ini {
	private:
		ParsingError errorBuffer;

		string	GetGameString	();
		string	GetLogFormat	();
	public:
		void	Load(fs::path file);		//Throws exception on fail.
		void	Save(fs::path file);		//Throws exception on fail.
	
		ParsingError ErrorBuffer();
		void ErrorBuffer(ParsingError buffer);
	};
}
#endif