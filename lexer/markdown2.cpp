#include <QDebug>
#include <QtMath>
#include <QRegularExpression>

#include <Qsci/qsciscintilla.h>

#include "../sqriptor.h"
#include "lexer/markdown2.h"

/*
Styles:
bold, **bold**
italic, *italic*, _italic_
stroke, ~~stroke~~
~~st**ro**ke~~
~~st_ro_ke~~
monospace, `monospace`
#* header
hr, ---
ol, "^\s*[0-9].\s"
ul, "^\s*\*\s"
tagopen, "<[^\s]*"
tagopentail, ">"
tagclose, "</[^\s]*>"
*/

namespace Style {
    enum Type {
        Default = 0, Header1, Header2, Header3, Header4, Header5, Header6,
        Monospace, StrikeMono, List, Ruler,
        // this is fun…
        Bold, Italic, Strike, BoldItalic, StrikeBold, StrikeItalic, StrikeBoldItalic,
        // same - hopefully
        Quote,
        BoldQuote, ItalicQuote, StrikeQuote, BoldItalicQuote, StrikeBoldQuote,
        StrikeItalicQuote, StrikeBoldItalicQuote,
        Bracket, Brace = Quote, Tag = Bracket
    };
};

QsciLexerMarkdown2::QsciLexerMarkdown2(QObject *parent) : QsciLexerCustom(parent)
{
    updateColors();
}

void QsciLexerMarkdown2::updateColors()
{
    QColor bg = Sqriptor::config.color.bg;
    QColor fg = Sqriptor::config.color.fg;
    QColor mid = QColor((2*bg.red()+fg.red())/3, (2*bg.green()+fg.green())/3, (2*bg.green()+fg.green())/3);
    setPaper(bg);
    setColor(fg);
    setColor(fg, Style::Default);
    setColor(Sqriptor::config.color.number, Style::List);
    setColor(Sqriptor::config.color.operateur, Style::Bracket);
    for (int i = Style::Quote; i <= Style::StrikeBoldItalicQuote; ++i)
        setColor(Sqriptor::config.color.string, i);
    setColor(mid, Style::Ruler);
    
    QFont font("sans", Sqriptor::config.font.pointSize());
    setFont(font);
    setFont(font, Style::Default);
    
    QFont bold = font; bold.setBold(true);
    setFont(bold, Style::Bold);
    setFont(bold, Style::BoldQuote);
//    bold.setStrikeOut(true);
//    setFont(bold, Style::StrikeBold);
//    setFont(bold, Style::StrikeBoldQuote);
    
    QFont italic = font; italic.setItalic(true);
    setFont(italic, Style::Italic);
    setFont(italic, Style::ItalicQuote);
//    italic.setStrikeOut(true);
//    setFont(italic, Style::StrikeItalic);
//    setFont(italic, Style::StrikeItalicQuote);
    
    QFont boldItalic = font; boldItalic.setBold(true); boldItalic.setItalic(true);
    setFont(boldItalic, Style::BoldItalic);
    setFont(boldItalic, Style::BoldItalicQuote);
//    boldItalic.setStrikeOut(true);
//    setFont(boldItalic, Style::StrikeBoldItalic);
//    setFont(boldItalic, Style::StrikeBoldItalicQuote);
    
//    QFont strike = font; strike.setStrikeOut(true);
//    setFont(strike, Style::Strike);
//    setFont(strike, Style::StrikeQuote);
    
    QFont mono = Sqriptor::config.font;
    setFont(mono, Style::Monospace);
    setColor(Sqriptor::config.color.keyword, Style::Monospace);
    mono.setStrikeOut(true);
    setFont(mono, Style::StrikeMono);
    setColor(Sqriptor::config.color.keyword, Style::StrikeMono);
    
    QFont header = font;
    float factor = 1.4;
    header.setBold(true);
    for (int i = Style::Header1; i <= Style::Header6; ++i) {
        header.setPointSize(qCeil(factor*font.pointSize()));
        setFont(header, i);
        if (i % 2) {
            header.setBold(false);
        } else {
            header.setBold(true);
            factor -= 0.1;
        }
    }
    
    setColor(mid, Style::Strike);
    setColor(mid, Style::StrikeBold);
    setColor(mid, Style::StrikeItalic);
    setColor(mid, Style::StrikeBoldItalic);
    
    fg = Sqriptor::config.color.string;
    mid = QColor((2*bg.red()+fg.red())/3, (2*bg.green()+fg.green())/3, (2*bg.green()+fg.green())/3);
    setColor(mid, Style::StrikeQuote);
    setColor(mid, Style::StrikeBoldQuote);
    setColor(mid, Style::StrikeItalicQuote);
    setColor(mid, Style::StrikeBoldItalicQuote);
}

bool operator==(const QStringView&view, const char* string) {
    return !view.compare(QLatin1String(string));
}

void QsciLexerMarkdown2::styleText(int start, int end)
{
    static const QRegularExpression tokenizer("######|#####|####|###|##|---|~~|"
                                              "\\*\\*\\*|\\*\\*|___|__|_|\\s+|[A-Za-z\\d]+|\\W");
    static const QRegularExpression nonWS("[^\\s]"); // \\W?
    static const QRegularExpression linebreak("\\s*(\\n|\\r)+\\s*");

    QString text = editor()->text(start, end);
    QRegularExpressionMatchIterator i = tokenizer.globalMatch(text/*, 0, QRegularExpression::PartialPreferFirstMatch*/);

    bool newline = true, quote = false, tag = false, bracket = false, brace = false, isInt;
    int header = 0, length = 0, style = 0;
    enum FontStyle { I1 = 0, B1, BI1, I2, B2, BI2, S, Mono, FontStyleCount };
    static const char *fsIndicator[FontStyleCount] = {"*", "**", "***", "_", "__", "___", "~~", "`"};
    auto finishStyle = [&]() {
        if (header) {
            setStyling(length, header);
        } else {
            if (style & (1<<Mono)) {
                setStyling(length, style & (1<<S) ? Style::StrikeMono : Style::Monospace);
            } else if (quote) {
                if (style & (1<<S)) {
                    if (style & (1<<I1|1<<I2)) setStyling(length, Style::StrikeItalicQuote);
                    else if (style & (1<<B1|1<<B2)) setStyling(length, Style::StrikeBoldQuote);
                    else if (style & (1<<BI1|1<<BI2)) setStyling(length, Style::StrikeBoldItalicQuote);
                    else setStyling(length, Style::StrikeQuote);
                } else {
                    if (style & (1<<I1|1<<I2)) setStyling(length, Style::ItalicQuote);
                    else if (style & (1<<B1|1<<B2)) setStyling(length, Style::BoldQuote);
                    else if (style & (1<<BI1|1<<BI2)) setStyling(length, Style::BoldItalicQuote);
                    else setStyling(length, Style::Quote);
                }
            } else if (style & (1<<S)) {
                if (style & (1<<I1|1<<I2)) setStyling(length, Style::StrikeItalic);
                else if (style & (1<<B1|1<<B2)) setStyling(length, Style::StrikeBold);
                else if (style & (1<<BI1|1<<BI2)) setStyling(length, Style::StrikeBoldItalic);
                else setStyling(length, Style::Strike);
            } else if (style & (1<<I1|1<<I2)) setStyling(length, Style::Italic);
            else if (style & (1<<B1|1<<B2)) setStyling(length, Style::Bold);
            else if (style & (1<<BI1|1<<BI2)) setStyling(length, Style::BoldItalic);
            else setStyling(length, Style::Default);
        }
        length = 0;
    };
    startStyling(start);
    QRegularExpressionMatch match, prev;
    while (i.hasNext()) {
        prev = match;
        match = i.next();
        QStringView token = match.capturedView(0);
#define NEXT_IS_WS i.hasNext() && !nonWS.match(i.peekNext().capturedView(0)).hasMatch()
        if (linebreak.match(token).hasMatch()) {
            if (header || quote) {
                finishStyle();
            }
            newline = true;
            header = quote = false;
            length += match.capturedLength(0);
            continue;
        }
        if (header) {
            length += token.toUtf8().length(); // match.capturedLength(0); - this sucks
            continue;
        }
        if (bracket) {
            if (token == "]") {
                setStyling(length+1, Style::Bracket); length = 0; bracket = false;
                continue;
            }
            length += token.toUtf8().length(); // match.capturedLength(0); - this sucks
            continue;
        }
        if (brace) {
            if (token == ")") {
                setStyling(length+1, Style::Brace); length = 0; brace = false;
                continue;
            }
            length += token.toUtf8().length(); // match.capturedLength(0); - this sucks
            continue;
        }
        if (tag) {
            if (token == ">") {
                if (length)
                    setStyling(length, Style::Quote);
                setStyling(1, Style::Tag); length = 0; tag = false;
                continue;
            }
            if (i.hasNext() && i.peekNext().capturedView(0) == "=") {
                setStyling(length, Style::Quote); // value?
                setStyling(token.toUtf8().length(), Style::List); // attribute
                setStyling(1, Style::Default); // "="
                length = 0;
                i.next(); // skip
                continue;
            }
            length += token.toUtf8().length(); // match.capturedLength(0); - this sucks
            continue;
        }
        if (newline && !quote) {
            if (token == "######" && NEXT_IS_WS) {
                finishStyle(); newline = false; header = 6;
            } else if (token == "#####" && NEXT_IS_WS) {
                finishStyle(); newline = false; header = 5;
            } else if (token == "####" && NEXT_IS_WS) {
                finishStyle(); newline = false; header = 4;
            } else if (token == "###" && NEXT_IS_WS) {
                finishStyle(); newline = false; header = 3;
            } else if (token == "##" && NEXT_IS_WS) {
                finishStyle(); newline = false; header = 2;
            } else if (token == "#" && NEXT_IS_WS) {
                finishStyle(); newline = false; header = 1;
            } else if (token == ">" && NEXT_IS_WS) {
                finishStyle(); newline = false; quote = true;
            } else if (token.startsWith(QLatin1String("---"))) { // TODO: not entirely accurate,  ---foobar is not a ruler, quotes?
                finishStyle(); newline = false; setStyling(3, Style::Ruler);
                continue;
            } else if ((token == "*" || token == "-" || token == "-") && NEXT_IS_WS) {
                finishStyle(); newline = false; setStyling(1, Style::List);
                continue;
            } else {
                Q_UNUSED(token.toInt(&isInt));
                if (isInt && i.hasNext() && i.peekNext().capturedView(0) == ".") { // ordered list item
                    finishStyle(); newline = false; setStyling(match.capturedLength(0) + 1, Style::List);
                    i.next(); // skip
                    continue;
                }
            }
        }
        
        if (newline && nonWS.match(token).hasMatch())
            newline = false;
        
        if (token == "[") {
            finishStyle();
            bracket = true;
        } else if (token == "(" && !length && prev.capturedView() == "]") {
            // !length because we just closed a bracket!
            // we therefore also don't need to finishStyle();
            brace = true;
        } else if (token == "<" && !(NEXT_IS_WS)) {
            finishStyle();
            length = 1;
            tag = true;
            while (!(NEXT_IS_WS)) {
                token = i.next().capturedView(0);
                if (token == ">") {
                    ++length;
                    tag = false;
                    break;
                }
                length += token.toUtf8().length(); // match.capturedLength(0); - this sucks
            }
            setStyling(length, Style::Tag);
            length = 0;
            continue;
        } else {
            for (int fs = 0; fs < FontStyleCount; ++fs) {
                if (token == fsIndicator[fs]) {
                    const bool nexWS = NEXT_IS_WS;
                    if ((style & (1<<fs)) && nonWS.match(prev.capturedView()).hasMatch()) {
                        length += match.capturedLength(0); // capture indicator
                        finishStyle();
                        length -= match.capturedLength(0); // sanitized below!
                        style &= ~(1<<fs);
                    } else if (!(style&(1<<fs)) && !nexWS) {
                        finishStyle();
                        style |= (1<<fs);
                    }
                    break;
                }
            }
        }
        length += token.toUtf8().length(); // match.capturedLength(0); - this sucks
    }
    if (length)
        finishStyle();
}