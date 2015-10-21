#include <boost/spirit/include/qi_match.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/phoenix_object.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <iterator>
#include <fstream>

namespace termcxx
{

namespace parser
{

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace px = boost::phoenix;

//using qi::double_;
using ascii::space;
using ascii::blank;
using ascii::graph;
using ascii::print;
//using px::ref;
using px::construct;

//using qi::eps;
//using qi::lit;
using qi::_val;
using qi::_1;
using ascii::char_;
using qi::eol;
using qi::eoi;
using qi::int_;


struct context
{
    int dummy;

    context () = default;
    context (context const &) = default;
    context (std::vector<char> a)
    { }
    context (std::vector<char> a, std::vector<char> b)
    { }
};

} }


BOOST_FUSION_ADAPT_STRUCT(
    termcxx::parser::context,
    (int, dummy))


namespace termcxx
{

namespace parser
{

template <typename Iterator>
struct parser
    : qi::grammar<Iterator, context()>
{
    qi::rule<Iterator, std::string() > eoleoi
    = eol | eoi
        ;

    // White space consists of the ' ' and <tab> character.
    qi::rule<Iterator, std::string() > white_space
    = char_(' ') | char_('\t')
        ;

    // An ALIAS may contain any graph characters other than ',','/' and
    // '|'. Graph characters are those characters for which isgraph() returns
    // non-zero.
    qi::rule<Iterator, std::string() > alias
    = +(graph - (char_(',') | char_('/') | char_('|')))
        ;

    // A LONGNAME may contain any print characters other than ',' and
    // '|'. Print characters are those characters for which isprint() returns
    // non-zero.
    qi::rule<Iterator, std::string() > longname
    = +(print - (char_(',') | char_('|')))
        ;

    // Common part of all three types of features factored out.
    qi::rule<Iterator, std::string() > feature_name
    = +(print - (char_(',') | char_('=') | char_('#')))
        ;

    // A BOOLEAN feature may contain any print characters other than ',', '=',
    // and '#'.
    qi::rule<Iterator, std::string() > boolean_feature
    = feature_name
        ;

    // A NUMERIC feature consists of:
    // a. A name which may contain any print character other than ',', '=', and
    //    '#'.
    // b. The '#' character.
    // c. A positive integer which conforms to the C language convention for
    //    integer constants.
    qi::rule<Iterator, std::string() > numeric_feature
    = feature_name >> char_('#') >> int_
        ;

    // c. A string which may contain any print characters other than ','.
    // (see also string_feature)
    qi::rule<Iterator, std::string() > string_feature_value
    = +(print - char_(','))
        ;

    // A STRING feature consists of:
    // a. A name which may contain any print character other than ',', '=', and
    //    '#'.
    // b. The '=' character.
    // c. A string which may contain any print characters other than ','.
    qi::rule<Iterator, std::string() > string_feature
    = feature_name >> char_('=') >> string_feature_value
        ;

    // White space immediately following a ',' is ignored.
    qi::rule<Iterator, std::string() > comma
    = char_(',') >> *white_space
        ;

    // feature : BOOLEAN
    //       | NUMERIC
    //       | STRING
    //       ;
    qi::rule<Iterator, std::string() > feature
    = boolean_feature
        | numeric_feature
        | string_feature
        ;

    // features : COMMA feature
    //       | features COMMA feature
    //       ;
    qi::rule<Iterator, std::string() > features
    = feature % comma
        ;

    // aliases : PIPE ALIAS
    //       | aliases PIPE ALIAS
    //       ;
    qi::rule<Iterator, std::string() > aliases
    = alias % char_('|')
        ;

    // Comments consist of <bol>, optional whitespace, a required '#', and a
    // terminating <eol>.
    qi::rule<Iterator, std::string() > comment_line
    = (*blank >> char_('#') >> *(char_ - eol) >> eoleoi)
        ;

    qi::rule<Iterator, std::string() > empty_line
    = ((+blank >> eoleoi)
        | eoleoi)
        ;

    qi::rule<Iterator, std::string() > def_first_line
    = (aliases >> eol)
        ;

    qi::rule<Iterator, std::string() > def_subsequent_line
    = (+white_space >> +features >> eoleoi)
        ;

    qi::rule<Iterator, std::string() > definition
    = (def_first_line >> *def_subsequent_line)
        ;

    qi::rule<Iterator, context()> start
    = (*(comment_line
            | empty_line
            | definition))[_val = construct<context> ()]
        ;

    parser()
        : parser::base_type(start)
    { }
};

//template struct parser<std::string::iterator>;
//template struct parser<std::istreambuf_iterator<char> >;

} // namespace parser

} // namespace termcxx


int
main ()
{
    typedef termcxx::parser::parser<std::istreambuf_iterator<char> > parser;
    parser p;

    std::ifstream f ("termtypes.ti");
    std::istreambuf_iterator<char> input_iterator(f);
    std::vector<char> input_buffer (input_iterator,
        std::istreambuf_iterator<char>());
    boost::spirit::qi::parse(input_buffer.begin(), input_buffer.end(), p);
}
