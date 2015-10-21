//#define BOOST_SPIRIT_DEBUG
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/qi.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <iterator>
#include <fstream>

namespace termcxx { namespace parser {
    namespace qi      = boost::spirit::qi;

    using Number      = long long;   // TODO FIXME SEHE - must support up to 99 digits (WTF)
    using Alias       = std::string;
    using Aliases     = std::vector<Alias>;

    using FeatureName = std::string;
    using String      = std::string;

    template <typename Value> struct GenericFeature {
        FeatureName name;
        Value       value;
    };

    using BoolFeature   = GenericFeature<bool>;
    using NumberFeature = GenericFeature<Number>;
    using StringFeature = GenericFeature<String>;

    using Feature       = boost::variant<BoolFeature, NumberFeature, StringFeature>;
    using Features      = std::vector<Feature>; // TODO FIXME SEHE map-like storage instead

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

            auto white_space     = copy(lit(' ')|'\t');
            eoleoi               = eol | eoi;
            caret_escape         = '^' >> no_case[control_chars];
            comma                = +(',' >> *white_space); // allow repeated `,,` as in line terminfo.ti:4603

            alias                = +(graph - char_(",/|")) >> &char_(",|");
            longname             = +(print - char_(",|"));
            feature_name         = +(print - char_(",=#"));
            string_feature_value = *(('\\' >> char_) | caret_escape | (print - ','));

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
                    (eoleoi)(comment_line)(empty_line)(comma)(caret_escape)
                    (alias)(longname)(feature_name)(boolean_feature)(numeric_feature)(string_feature_value)
                    (string_feature)(feature)(features)(aliases)(header_line)(feature_line)(description)(descriptions)
                )
        }

        qi::symbols<char, char> control_chars;
        qi::rule<Iterator, char()> caret_escape;
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

int main() {
    using It = boost::spirit::istream_iterator;
    typedef termcxx::parser::parser<It> parser;
    parser p;

    std::ifstream f("work.ti");
    It first(f >> std::noskipws), end;
    termcxx::parser::Descriptions descriptions;
    bool ok = boost::spirit::qi::parse(first, end, p, descriptions);

    if (ok)
    {
        std::cout << "Yay\n";
        std::cout << "Parsed " << descriptions.size() << " descriptions\n";

        for (auto& d : descriptions) {
            std::copy(d.aliases.begin(), d.aliases.end(), std::ostream_iterator<std::string>(std::cout, "|"));
            std::cout << ", \n";

            for (auto& f : d.features) {
                std::cout << "\t" << "XXX, \n";
            }
        }
    }
    else
        std::cout << "Nay\n";

    if (first!=end)
        std::cout << "We have unparsed trailing input: '" << std::string(first, end) << "'\n";
}
