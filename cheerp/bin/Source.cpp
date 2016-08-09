#include <iostream>
#include <cheerp/client.h>
#include <cheerp/clientlib.h>
#include "path/Unuseful.h"
#include <cstring>
#include <cheerp/webgl.h>

using namespace std;
using namespace client;

int webMain() {
	int i;
    client::console.log("kjadkjdk");
    //char str1[10] = "Hello";
    //Unuseful u; 
    //client::console.log(u.printUnusefulStatement());
    //client::HTMLElement * body = document.get_body();
	 //Prepare a string
     HTMLElement* body=document.get_body();
	 std::string original_text = "hello, world!";
	 //Create a new <h1> element, and set it's content to the string
	 HTMLElement * textDisplay = document.createElement("h1");
	 textDisplay->set_textContent(original_text.c_str());
	 //Create a text input element <input type="text">
	 HTMLInputElement * inputBox = static_cast<HTMLInputElement*>(document.createElement("input") );
	 inputBox->setAttribute("type", "text");
	 //We can also style it here, but CSS would be better
	 inputBox->setAttribute("style", "width:200px");
	 //This sets the default value
	 inputBox->setAttribute("value", original_text.c_str() );
	 //Set up the handler for the input event. Use a C++11 lambda to capture the variables we need
	 //Add the new elements to the <body>
	 body->appendChild( textDisplay );
	 body->appendChild( inputBox );
    return 0;
}

