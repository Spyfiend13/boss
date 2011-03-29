/*	Better Oblivion Sorting Software
	
	Quick and Dirty Load Order Utility
	(Making C++ look like the scripting language it isn't.)

    Copyright (C) 2009-2010  Random/Random007/jpearce & the BOSS development team
    http://creativecommons.org/licenses/by-nc-nd/3.0/

	$Revision: 2188 $, $Date: 2011-01-20 10:05:16 +0000 (Thu, 20 Jan 2011) $
*/

//Header file for userlist grammar definition.

/* Need to:
1. Get error handling working properly (currently throws silent exceptions and doesn't specify area).
2. Make error reports useful.
*/
#ifndef __BOSS_USERLIST_GRAM_H__
#define __BOSS_USERLIST_GRAM_H__

#ifndef BOOST_SPIRIT_UNICODE
#define BOOST_SPIRIT_UNICODE 
#endif

#include "Parsing/Data.h"
#include "Parsing/Skipper.h"
#include "Common/Globals.h"
#include "Support/Helpers.h"
#include "Support/Logger.h"

#include <sstream>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/home/phoenix/object/construct.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

namespace boss {
	namespace unicode = boost::spirit::unicode;
	namespace phoenix = boost::phoenix;
	namespace qi = boost::spirit::qi;

	using namespace std;
	using namespace qi::labels;

	using qi::skip;
	using qi::eol;
	using qi::lexeme;
	using qi::on_error;
	using qi::fail;

	using unicode::char_;
	using unicode::no_case;

	using boost::spirit::info;
	using boost::format;

	////////////////////////////
	//Userlist Grammar.
	////////////////////////////

	// Error messages for rule validation
	//Syntax errors
	static format ESortLineInForRule("includes a sort line in a rule with a FOR rule keyword.");
	static format EPluginNotInstalled("references '%1%', which is not installed.");
	static format EAddingModGroup("tries to add a group.");
	static format ESortingGroupEsms("tries to sort the group \"ESMs\".");
	static format ESortingMasterEsm("tries to sort the master .ESM file.");
	static format EReferencingModAndGroup("references a mod and a group.");
	static format ESortingGroupBeforeEsms("tries to sort a group before the group \"ESMs\".");
	static format ESortingModBeforeGameMaster("tries to sort a mod before the master .ESM file.");
	static format EInsertingToTopOfEsms("tries to insert a mod into the top of the group \"ESMs\", before the master .ESM file.");
	static format EInsertingGroupOrIntoMod("tries to insert a group or insert something into a mod.");
	static format EAttachingMessageToGroup("tries to attach a message to a group.");
	//Grammar errors - not yet implemented.
	static format ERuleHasUndefinedObject("The line with keyword '%1%' has an undefined object.");
	static format EUnrecognisedKeyword("The line \"%1%: %2%\" does not contain a recognised keyword. If this line is the start of a rule, that rule will also be skipped.");
	static format EAppearsBeforeFirstRule("The line \"%1%: %2%\" appears before the first recognised rule line. Line skipped.");
	static format EUnrecognizedKeywordBeforeFirstRule("The line \"%1%: %2%\" does not contain a recognised keyword, and appears before the first recognised rule line. Line skipped.");

	//Syntax error formatting.
	static format SyntaxErrorFormat("<p class='error'>"
		"Syntax Error: The rule beginning \"%1%: %2%\" %3%"
		"</p>\n\n");

	static format ParsingErrorFormat("<p><span class='error'>Parsing Error: Expected a %1% at:</span>"
		"<blockquote style='font-style:italic;'>%2%</blockquote>"
		"<span class='error'>Userlist parsing aborted. No rules will be applied.</span></p>\n\n");

	void AddSyntaxError(keyType const& rule, string const& object, format const& message) {
		string keystring = KeyToString(rule);
		string const msg = (SyntaxErrorFormat % keystring % object % message.str()).str();
		errorMessageBuffer.push_back(msg);
		return;
	}

	// Used to throw as exception when signaling a userlist syntax error, in order to make the code a bit more compact.
	struct failure {
		failure(keyType const& ruleKey, string const& ruleObject, format const& message) 
			: ruleKey(ruleKey), ruleObject(ruleObject), message(message) {}

		keyType ruleKey;
		string ruleObject;
		format message;
	};

	//Rule checker function, checks for syntax (not parsing) errors.
	void RuleSyntaxCheck(vector<rule>& userlist, rule currentRule) {
		bool skip = false;
		try {
			keyType ruleKey = currentRule.ruleKey;
			string ruleObject = currentRule.ruleObject;
			if (IsPlugin(ruleObject)) {
				if (!Exists(data_path / ruleObject))
					throw failure(ruleKey, ruleObject, EPluginNotInstalled % ruleObject);
				if (IsMasterFile(ruleObject))
					throw failure(ruleKey, ruleObject, ESortingMasterEsm);
			} else {
				if (Tidy(ruleObject) == "esms")
					throw failure(ruleKey, ruleObject, ESortingGroupEsms);
				if (ruleKey == ADD && !IsPlugin(ruleObject))
					throw failure(ruleKey, ruleObject, EAddingModGroup);
				else if (ruleKey == FOR)
					throw failure(ruleKey, ruleObject, EAttachingMessageToGroup);
			}
			for (size_t i=0; i<currentRule.lines.size(); i++) {
				keyType key = currentRule.lines[i].key;
				string subject = currentRule.lines[i].object;
				if (key == BEFORE || key == AFTER) {
					if (currentRule.ruleKey == FOR)
						throw failure(ruleKey, ruleObject, ESortLineInForRule);
					if ((IsPlugin(ruleObject) && !IsPlugin(subject)) || (!IsPlugin(ruleObject) && IsPlugin(subject)))
						throw failure(ruleKey, ruleObject, EReferencingModAndGroup);
					if (key == BEFORE) {
						if (Tidy(subject) == "esms")
							throw failure(ruleKey, ruleObject, ESortingGroupBeforeEsms);
						else if (IsMasterFile(subject))
							throw failure(ruleKey, ruleObject, ESortingModBeforeGameMaster);
					}
				} else if (key == TOP || key == BOTTOM) {
					if (ruleKey == FOR)
						throw failure(ruleKey, ruleObject, ESortLineInForRule);
					if (key == TOP && Tidy(subject)=="esms")
						throw failure(ruleKey, ruleObject, EInsertingToTopOfEsms);
					if (!IsPlugin(ruleObject) || IsPlugin(subject))
						throw failure(ruleKey, ruleObject, EInsertingGroupOrIntoMod);
				} else if (key == APPEND || key == REPLACE) {
					if (!IsPlugin(ruleObject))
						throw failure(ruleKey, ruleObject, EAttachingMessageToGroup);
				}
			}
		} catch (failure const& e) {
			skip = true;
			AddSyntaxError(e.ruleKey, e.ruleObject, e.message);
			string const keystring = KeyToString(e.ruleKey);
			LOG_ERROR("Userlist Syntax Error: The rule beginning \"%s: %s\" %s", keystring.c_str(), e.ruleObject.c_str(), e.message.str().c_str());
		}
		if (!skip)
			userlist.push_back(currentRule);
		return;
	}

	template <typename Iterator>
	struct userlist_grammar : qi::grammar<Iterator, vector<rule>(), Skipper<Iterator> > {
		userlist_grammar() : userlist_grammar::base_type(ruleList, "userlist grammar") {

			//A list is a vector of rules. Rules are separated by line endings.
			ruleList = userlistRule[phoenix::bind(&RuleSyntaxCheck, _val, _1)] % +eol; 

			//A rule consists of a rule line containing a rule keyword and a rule object, followed by one or more message or sort lines.
			userlistRule %=
				ruleKey > ':' > object
				>> +eol
				> sortOrMessageLine % +eol;

			sortOrMessageLine %=
				sortOrMessageKey
				> ':'
				> object;

			object %= lexeme[skip("//" >> *(char_ - eol))[+(char_ - eol)]]; //String, but skip comment if present.

			ruleKey %= no_case[ruleKeys];

			sortOrMessageKey %= no_case[sortOrMessageKeys];

			//Give each rule names.
			ruleList.name("rules");
			userlistRule.name("rule");
			sortOrMessageLine.name("sort or message line");
			object.name("line object");
			ruleKey.name("rule keyword");
			sortOrMessageKey.name("sort or message keyword");
		
			on_error<fail>(ruleList,phoenix::bind(&userlist_grammar<Iterator>::SyntaxError,this,_1,_2,_3,_4));
			on_error<fail>(userlistRule,phoenix::bind(&userlist_grammar<Iterator>::SyntaxError,this,_1,_2,_3,_4));
			on_error<fail>(sortOrMessageLine,phoenix::bind(&userlist_grammar<Iterator>::SyntaxError,this,_1,_2,_3,_4));
			on_error<fail>(object,phoenix::bind(&userlist_grammar<Iterator>::SyntaxError,this,_1,_2,_3,_4));
			on_error<fail>(ruleKey,phoenix::bind(&userlist_grammar<Iterator>::SyntaxError,this,_1,_2,_3,_4));
			on_error<fail>(sortOrMessageKey,phoenix::bind(&userlist_grammar<Iterator>::SyntaxError,this,_1,_2,_3,_4));
		}

		typedef Skipper<Iterator> skipper;

		qi::rule<Iterator, vector<rule>(), skipper> ruleList;
		qi::rule<Iterator, rule(), skipper> userlistRule;
		qi::rule<Iterator, line(), skipper> sortOrMessageLine;
		qi::rule<Iterator, keyType(), skipper> ruleKey, sortOrMessageKey;
		qi::rule<Iterator, string(), skipper> object;
	
		void SyntaxError(Iterator const& first, Iterator const& last, Iterator const& errorpos, info const& what) {
			ostringstream out;
			out << what;
			string expect = out.str().substr(1,out.str().length()-2);
			if (expect == "eol")
				expect = "end of line";

			string context(errorpos, min(errorpos +50, last));
			boost::trim_left(context);

			LOG_ERROR("Userlist Parsing Error: Expected a %s at \"%s\". Userlist parsing aborted. No rules will be applied.", expect.c_str(), context.c_str());
			
			expect = "&lt;" + expect + "&gt;";
			boost::replace_all(context, "\n", "<br />\n");
			string msg = (ParsingErrorFormat % expect % context).str();
			errorMessageBuffer.push_back(msg);
			return;
		}
	};
}


#endif