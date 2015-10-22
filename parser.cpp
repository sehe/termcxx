//#define BOOST_SPIRIT_DEBUG
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/spirit/include/qi.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <iterator>
#include <fstream>

namespace termcxx { namespace parser {
    namespace qi      = boost::spirit::qi;

    using Bool        = bool;
    using Number      = boost::multiprecision::cpp_int;
    using Alias       = std::string;
    using Aliases     = std::vector<Alias>;

    using FeatureName = std::string;
    using String      = std::string;

    template <typename Value> struct GenericFeature {
        FeatureName name;
        Value       value;

      private:
        friend std::ostream& operator<<(std::ostream& os, GenericFeature const& gf) {
            os << gf.name;
            print_value(os, gf.value);
            return os;
        }

        static void print_value(std::ostream& os, Bool)     { }
        static void print_value(std::ostream& os, Number v) { os << "#" << v; }
        static void print_value(std::ostream& os, String v) { 
            os << "=";
            for(uint8_t ch : v) {
                if (isprint(ch) && !strchr(",!#\\^", ch)) os << ch;
                else {
                    char octals[4] = { "000" }, *out = octals+2;
                    while (ch) { *out-- = '0' + (ch % 8); ch /= 8; }
                    os << "\\" << octals;
                }
            }
        }
    };

    using BoolFeature   = GenericFeature<Bool>;
    using NumberFeature = GenericFeature<Number>;
    using StringFeature = GenericFeature<String>;

    using Feature       = boost::variant<BoolFeature, NumberFeature, StringFeature>;
    using Features      = std::vector<Feature>;

    struct Description {
        Aliases  aliases;
        Features features;
    };

    using Descriptions = std::vector<Description>;
} }

BOOST_FUSION_ADAPT_TPL_STRUCT((Value), (termcxx::parser::GenericFeature) (Value), name, value)
BOOST_FUSION_ADAPT_STRUCT(termcxx::parser::Description, aliases, features)

namespace termcxx { namespace parser {

    template <typename Iterator>
    struct parser : qi::grammar<Iterator, Descriptions()>
    {
        parser() : parser::base_type(descriptions)
        {
            using namespace qi;

            // qi::symbols:
            //      When symbols is used for case-insensitive parsing (in a no_case directive), added symbol strings
            //      should be in lowercase. Symbol strings containing one or more uppercase characters will not match
            //      any input when symbols is used in a no_case directive.
            control_chars.add // see https://en.wikipedia.org/wiki/C0_and_C1_control_codes
                ("@", 0x00) ("a", 0x01) ("b", 0x02) ("c", 0x03) ("d",  0x04) ("e", 0x05) ("f", 0x06) ("g", 0x07)
                ("h", 0x08) ("i", 0x09) ("j", 0x0a) ("k", 0x0b) ("l",  0x0c) ("m", 0x0d) ("n", 0x0e) ("o", 0x0f)
                ("p", 0x10) ("q", 0x11) ("r", 0x12) ("s", 0x13) ("t",  0x14) ("u", 0x15) ("v", 0x16) ("w", 0x17)
                ("x", 0x18) ("y", 0x19) ("z", 0x1a) ("[", 0x1b) ("\\", 0x1c) ("]", 0x1d) ("^", 0x1e) ("_", 0x1f)
                // WP: not technically part of the control characters but exists
                ("?", 0x7f);

            char_escape = '\\' > (
                    'a' >> attr('\a')   // Alert
                  | 'b' >> attr('\b')   // Backspace
                  | 'E' >> attr('\x1b') // An ESCAPE character
                  | 'e' >> attr('\x1b') // An ESCAPE character
                  | 'f' >> attr('\f')   // Form feed
                  | 'l' >> attr('\n')   // Linefeed
                  | 'n' >> attr('\n')   // Newline
                  | 'r' >> attr('\r')   // Carriage return
                  | 's' >> attr(' ')    // Space
                  | 't' >> attr('\t')   // Tab
                  | '^' >> attr('^')    // Caret (^)
                  | char_('\\')         // Backslash (\)
                  | ',' >> attr(',')    // Comma (,)
                  | ':' >> attr(':')    // Colon (:)
                  // branch ordering is important here!
                  | uint_parser<uint8_t, 8, 3, 3>() // Any character, specified as three octal digits
                  | '0' >> attr('\0')               // Null
                );

            auto white_space     = copy(lit(' ')|'\t');
            eoleoi               = eol | eoi;
            caret_escape         = '^' >> no_case[control_chars];
            comma                = ',' >> *white_space;

            alias                = +(graph - char_(",/|")) >> &char_(",|");
            longname             = +(print - char_(",|"));
            feature_name         = +(print - char_(",=#"));
            string_feature_value = *(char_escape | caret_escape | (print - ','));

            boolean_feature      = feature_name >> qi::attr(true);
            numeric_feature      = feature_name >> '#' >> int_;
            string_feature       = feature_name >> '=' >> string_feature_value;

            feature              = numeric_feature | string_feature | boolean_feature;
            features             = +(!eol >> feature >> comma);

            aliases              = alias % '|' >> -('|' >> longname);
            header_line          = (aliases >> comma >> eol);
            feature_line         = +white_space >> features >> eoleoi;
            description          = header_line >> *feature_line;

            comment_line         = *blank >> '#' >> *(char_ - eol) >> eoleoi;
            empty_line           = !eoi >> *blank >> eoleoi;
            descriptions         = skip(comment_line | empty_line) [ *lexeme[ description ] ];

            BOOST_SPIRIT_DEBUG_NODES(
                    (eoleoi)(comment_line)(empty_line)(comma)(caret_escape)(char_escape)
                    (alias)(longname)(feature_name)(boolean_feature)(numeric_feature)(string_feature_value)
                    (string_feature)(feature)(features)(aliases)(header_line)(feature_line)(description)(descriptions)
                )
        }

        qi::symbols<char, char> control_chars;
        qi::rule<Iterator, char()> caret_escape, char_escape;
        qi::rule<Iterator> eoleoi, comment_line, empty_line, comma;

        qi::rule<Iterator, Alias()>         alias;
        qi::rule<Iterator, std::string()>   longname;
        qi::rule<Iterator, std::string()>   feature_name;
        qi::rule<Iterator, BoolFeature()>   boolean_feature;
        qi::rule<Iterator, NumberFeature()> numeric_feature;
        qi::rule<Iterator, String()>        string_feature_value;

        qi::rule<Iterator, StringFeature()> string_feature;
        qi::rule<Iterator, Feature()>       feature;
        qi::rule<Iterator, Features()>      features, feature_line;
        qi::rule<Iterator, Aliases()>       aliases, header_line;
        qi::rule<Iterator, Description()>   description;
        qi::rule<Iterator, Descriptions()>  descriptions;
    };

    //template struct parser<std::string::iterator>;
    //template struct parser<boost::spirit::istream_iterator>;

} } // namespace parser, termcxx

int main(int argc, char** argv) {
    using It = boost::spirit::istream_iterator;
    termcxx::parser::parser<It> const parser;

    for (auto s : std::vector<std::string> { argv+1, argv+argc }) 
    {
        std::cerr << "Parsing " << s << "\n";
        std::ifstream f(s);
        It first(f >> std::noskipws), end;

        termcxx::parser::Descriptions descriptions;
        bool ok = boost::spirit::qi::parse(first, end, parser, descriptions);

        if (ok)
        {
            std::cerr << "# Parsed " << descriptions.size() << " descriptions\n";

            for (auto& d : descriptions) {
                assert(d.aliases.size()>0);
                std::copy_n(d.aliases.begin(), d.aliases.size() - 1, std::ostream_iterator<std::string>(std::cout, "|"));
                std::cout << d.aliases.back() << ", \n";

                for (auto& f : d.features) {
                    std::cout << "\t" << f << ",\n";
                }
            }
        }
        else {
            std::cout << "# Parse failuer\n";
        }

        if (first!=end) {
            std::cerr << "We have unparsed trailing input: '" << std::string(first, end) << "'\n";
        }
    }
}
