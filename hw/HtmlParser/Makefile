test:  HtmlParserDemo NYTimes.html
	./HtmlParserDemo t NYTimes.html > TitleWords.txt
	diff -Z TitleWords.correct.txt TitleWords.txt
	./HtmlParserDemo w NYTimes.html > Words.txt
	diff -Z Words.correct.txt Words.txt 
	./HtmlParserDemo a NYTimes.html > Links.txt
	diff -Z Links.correct.txt Links.txt 
	./HtmlParserDemo b NYTimes.html > Base.txt
	diff -Z Base.correct.txt Base.txt 

HtmlParserDemo:  HtmlTags.h HtmlTags.cpp HtmlParser.h HtmlParser.cpp HtmlParserDemo.cpp
	g++ -std=c++17 HtmlParserDemo.cpp HtmlTags.cpp HtmlParser.cpp -DLinux -o HtmlParserDemo

clean:
	rm -f HtmlParserDemo TitleWords.txt Words.txt Links.txt Base.txt
