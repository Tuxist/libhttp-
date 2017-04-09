#include <iostream>
#include <sstream>
#include "../src/exception.h"

#include "http.h"
#include "httpd.h"

void sendResponse(libhttppp::Connection *curcon,libhttppp::HttpRequest *curreq) {
     libhttppp::HttpResponse curres;
     curres.setState(HTTP200);
     curres.setVersion(HTTPVERSION(1.1));
     curres.setContentType("text/html");
     std::stringstream condat;
     condat  << "<!DOCTYPE HTML>"
             << " <html>"
             << "  <head>"
             << "    <title>ConnectionTest</title>"
             << "    <meta content=\"\">"
             << "    <meta charset=\"utf-8\">"
             << "    <style></style>"
             << "  </head>"
             << "<body>"
             
             << "<div style=\"border: thin solid black\">"
             << "<h2>Get Request</h2>"
             << "<form action=\"/\" method=\"get\">"
             << "First name:<br> <input type=\"text\" name=\"firstname\" value=\"test\"><br>"
             << "Last name:<br>  <input type=\"text\" name=\"lastname\" value=\"test\"><br>"
             << "<button type=\"submit\">Submit</button>"
             << "</form>"
             << "</div><br/>"
             
             << "<div style=\"border: thin solid black\">"
             << "<h2>Post Request</h2>"
             << "<form action=\"/\" method=\"post\">"
             << "First name:<br> <input type=\"text\" name=\"firstname\" value=\"test\"><br>"
             << "Last name:<br> <input type=\"text\" name=\"lastname\" value=\"test\"><br>"
             << "<button type=\"submit\">Submit</button>"
             << "</form>"
             << "</div></br>"
             
             << "<div style=\"border: thin solid black\">"
             << "<h2>Post Multiform Request</h2>"
             << "<form action=\"/\" method=\"post\" enctype=\"multipart/form-data\" >"
             << "First name:<br> <input type=\"text\" name=\"firstname\" value=\"test\"><br>"
             << "Last name:<br> <input type=\"text\" name=\"lastname\" value=\"test\"><br>"
             << "<button type=\"submit\">Submit</button>"
             << "</form>"
             << "</div></br>"
            
             << "<div style=\"border: thin solid black\">"
             << "<h2>Post Multiform File upload</h2>"
             << "<form action=\"/\" method=\"post\" enctype=\"multipart/form-data\" >"
             << "File name:<br><input name=\"datei\" type=\"file\"><br>"
             << "<button type=\"submit\">Submit</button>"
             << "</form>"
             << "</div></br>"
             
             << "<div style=\"border: thin solid black\">"
             << "<h2>Post Multiform mutiple File upload</h2>"
             << "<form action=\"/\" method=\"post\" enctype=\"multipart/form-data\" >"
             << "File name:<br><input name=\"datei\" type=\"file\" multiple><br>"
             << "<button type=\"submit\">Submit</button>"
             << "</form>"
             << "</div></br>"
             
             << "<div style=\"border: thin solid black\">"
             << "<h2>Encoding Test</h2>"
             << "<form action=\"/\" method=\"post\">"
             << "First name:<br> <input type=\"text\" name=\"encoding\" value=\"&=\" readonly><br>"
             << "<button type=\"submit\">Submit</button>"
             << "</form>"
             << "</div></br>";
     libhttppp::HttpForm curform;
     curform.parse(curreq); 
     
     condat  << "<div style=\"border: thin solid black\">"
             << "<h2>Output</h2>";
      if(curform.getBoundary())
        condat  << "Boundary: " << curform.getBoundary();
     condat  << "</div></body></html>";
     std::string buffer=condat.str();
     curres.send(curcon,buffer.c_str(),buffer.length());
};

class Controller : public libhttppp::Queue {
public:
  Controller(libhttppp::ServerSocket* serversocket) : Queue(serversocket){
    
  };
  void RequestEvent(libhttppp::Connection *curcon){
   try{
     std::cerr << "Parse Request\n";
     libhttppp::HttpRequest curreq;
     curreq.parse(curcon);
     std::cerr << "Send answer\n";
     sendResponse(curcon,&curreq);
   }catch(libhttppp::HTTPException &e){
     std::cerr << e.what() << "\n";
     throw e;
   }
  }
private:
  
};



class HttpConD : public libhttppp::HttpD {
public:
  HttpConD(int argc, char** argv) : HttpD(argc,argv){
    libhttppp::HTTPException httpexception;
    try {
      Controller controller(getServerSocket());
      controller.runEventloop();
    }catch(libhttppp::HTTPException &e){
      std::cerr << e.what() << "\n";
    }
  };
private:
};

int main(int argc, char** argv){
  HttpConD(argc,argv);
} 
