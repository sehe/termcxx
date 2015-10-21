#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_match.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <iterator>
#include <fstream>

namespace termcxx { namespace parser {
    namespace qi    = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
    namespace px    = boost::phoenix;

    struct context
    {
        //int dummy;

        //context () = default;
        //context (context const &) = default;
        //context (std::vector<char>) { }
        //context (std::vector<char>, std::vector<char>) { }
    };
} }

//BOOST_FUSION_ADAPT_STRUCT(termcxx::parser::context, dummy)

namespace termcxx { namespace parser {

    template <typename Iterator, typename Skipper = qi::blank_type>
    struct parser : qi::grammar<Iterator, context()>
    {
        parser() : parser::base_type(start)
        {
            using namespace qi;

            eoleoi
                = eol | eoi
                ;

            // White space consists of the ' ' and <tab> character.
            white_space
                = char_(' ') | char_('\t')
                ;

            // An ALIAS may contain any graph characters other than ',','/' and
            // '|'. Graph characters are those characters for which isgraph() returns
            // non-zero.
            alias
                = +(graph - char_(",/|"))
                ;

            // A LONGNAME may contain any print characters other than ',' and
            // '|'. Print characters are those characters for which isprint() returns
            // non-zero.
            longname
                = +(print - (char_(',') | char_('|')))
                ;

            // Common part of all three types of features factored out.
            feature_name
                = +(print - (char_(',') | char_('=') | char_('#')))
                ;

            // A BOOLEAN feature may contain any print characters other than ',', '=',
            // and '#'.
            boolean_feature
                = feature_name
                ;

            // A NUMERIC feature consists of:
            // a. A name which may contain any print character other than ',', '=', and
            //    '#'.
            // b. The '#' character.
            // c. A positive integer which conforms to the C language convention for
            //    integer constants.
            numeric_feature
                = feature_name >> char_('#') >> int_
                ;

            // c. A string which may contain any print characters other than ','.
            // (see also string_feature)
            string_feature_value
                = +(print - char_(','))
                ;

            // A STRING feature consists of:
            // a. A name which may contain any print character other than ',', '=', and
            //    '#'.
            // b. The '=' character.
            // c. A string which may contain any print characters other than ','.
            string_feature
                = feature_name >> char_('=') >> string_feature_value
                ;

            // White space immediately following a ',' is ignored.
            comma
                = char_(',') >> *white_space
                ;

            // feature : BOOLEAN
            //       | NUMERIC
            //       | STRING
            //       ;
            feature
                = boolean_feature
                | numeric_feature
                | string_feature
                ;

            // features : COMMA feature
            //       | features COMMA feature
            //       ;
            features
                = feature % comma
                ;

            // aliases : ALIAS // SEHE confusing, fixed?
            //       | aliases PIPE ALIAS
            //       ;
            aliases
                = alias % '|'
                ;

            // Comments consist of <bol>, optional whitespace, a required '#', and a
            // terminating <eol>.
            comment_line
                = (*blank >> char_('#') >> *(char_ - eol) >> eoleoi)
                ;

            empty_line
                = ((+blank >> eoleoi)
                        | eoleoi)
                ;

            def_first_line
                = (aliases >> eol)
                ;

            def_subsequent_line
                = (+white_space >> +features >> eoleoi)
                ;

            definition
                = (def_first_line >> *def_subsequent_line)
                ;

            start
                = (*(comment_line
                            | empty_line
                            | definition)) [_val = px::construct<context> ()]
                ;
        }

        qi::rule<Iterator, std::string()>  eoleoi;
        qi::rule<Iterator, std::string()>  white_space;
        qi::rule<Iterator, std::string()>  alias;
        qi::rule<Iterator, std::string()>  longname;
        qi::rule<Iterator, std::string()>  feature_name;
        qi::rule<Iterator, std::string()>  boolean_feature;
        qi::rule<Iterator, std::string()>  numeric_feature;
        qi::rule<Iterator, std::string()>  string_feature_value;
        qi::rule<Iterator, std::string()>  string_feature;
        qi::rule<Iterator, std::string()>  comma;
        qi::rule<Iterator, std::string()>  feature;
        qi::rule<Iterator, std::string()>  features;
        qi::rule<Iterator, std::string()>  aliases;
        qi::rule<Iterator, std::string()>  comment_line;
        qi::rule<Iterator, std::string()>  empty_line;
        qi::rule<Iterator, std::string()>  def_first_line;
        qi::rule<Iterator, std::string()>  def_subsequent_line;
        qi::rule<Iterator, std::string()>  definition;
        qi::rule<Iterator, context()>      start;
    };

    //template struct parser<std::string::iterator>;
    //template struct parser<boost::spirit::istream_iterator>;

} } // namespace parser, termcxx

int
main ()
{
    using It = boost::spirit::istream_iterator;
    typedef termcxx::parser::parser<It> parser;
    parser p;

    std::ifstream f ("termtypes.ti");
    It first(f >> std::noskipws), end;
    bool ok = boost::spirit::qi::parse(first, end, p);

    if (ok)
        std::cout << "Yay\n";
    else
        std::cout << "Nay\n";

    if (first!=end)
        std::cout << "We have unparsed trailing input: '" << std::string(first, end) << "'\n";
}
