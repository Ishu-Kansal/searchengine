test:  HtmlParserDemo NYTimes.html
	./HtmlParserDemo t NYTimes.html > TitleWords.txt
	diff -Z TitleWords.correct.txt TitleWords.txt
	./HtmlParserDemo w NYTimes.html > Words.txt
	diff -Z Words.correct.txt Words.txt 
	./HtmlParserDemo a NYTimes.html > Links.txt
	diff -Z Links.correct.txt Links.txt 
	./HtmlParserDemo b NYTimes.html > Base.txt
	diff -Z Base.correct.txt Base.txt 

HTML_FILE = reddit.html

test2:  HtmlParserDemo $(HTML_FILE)
	./HtmlParserDemo t $(HTML_FILE) > TitleWords.txt
	./HtmlParserDemo w $(HTML_FILE) > Words.txt
	./HtmlParserDemo a $(HTML_FILE) > Links.txt
	./HtmlParserDemo b $(HTML_FILE) > Base.txt

HtmlParserDemo: HtmlTags.h HtmlTags.cpp HtmlParser.h HtmlParser.cpp HtmlParserDemo.cpp
	g++ -std=c++20 HtmlParserDemo.cpp HtmlTags.cpp HtmlParser.cpp -DLinux -o HtmlParserDemo

RunHtmlParser: HtmlTags.h HtmlTags.cpp HtmlParser.h HtmlParser.cpp RunHtmlParser.cpp
	g++ -std=c++20 RunHtmlParser.cpp HtmlTags.cpp HtmlParser.cpp -DLinux -o RunHtmlParser

clean:
	rm -f HtmlParserDemo TitleWords.txt Words.txt Links.txt Base.txt
