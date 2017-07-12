function createXHR()   
{  
    var xhr;  
    try   
    {  
        xhr = new ActiveXObject("Msxml2.XMLHTTP");  
    }   
    catch (e)   
    {  
        try   
        {  
            xhr = new ActiveXObject("Microsoft.XMLHTTP");  
        }  
        catch(E)   
        {  
            xhr = false;  
        }  
    }  
  
    if (!xhr && typeof XMLHttpRequest != 'undefined')   
    {  
        xhr = new XMLHttpRequest();  
    }  
  
    return xhr;  
}  

function chtext()   
{  
    xhr = createXHR();  
    if(xhr)  
    {  
        xhr.onreadystatechange=callbackFunction;  
  //    	alert("open()");
        xhr.open("GET", "sys.cgi?cur_time=" + new Date().getTime());  
      
//		alert("send()");
        xhr.send(null);  
    }  
    else  
    {  
        alert("createXHR() error!");  
    }  
}  
  
function callbackFunction()  
{  
    if (xhr.readyState == 4)   
    {  
        if (xhr.status == 200)   
        {  
			//alert("callback()");
            var returnValue = xhr.responseText;  
			//document.writeln(returnValue);
  
            if(returnValue != null && returnValue.length > 0)  
            { 
				//alert(returnValue);
                document.getElementById("CPU").value = returnValue.split(";")[0];  
                document.getElementById("MEM").value = returnValue.split(";")[1];  
                document.getElementById("DISK").value = returnValue.split(";")[2];  
                document.getElementById("MAC").value = returnValue.split(";")[3];  
                document.getElementById("IP").value = returnValue.split(";")[4];  
				document.getElementById("upresult").innerHTML=returnValue.split(";")[5];
				//document.writeln(document.getElementById("MEM").value);
            }  
            else  
            {  
                //alert("result is NULL !");  
            }  
        }   
        else   
        {  
            //alert("page error!");  
        }  
    }  
}  

function upfilcallback()  
{  
    if (xhr.readyState == 4)   
    {  
        if (xhr.status == 200)   
        {  
			//alert("callback()");
            var returnValue = xhr.responseText;  
			//document.writeln(returnValue);
  
            if(returnValue != null && returnValue.length > 0)  
            { 
				//alert(returnValue);
				//document.writeln(document.getElementById("MEM").value);
				document.getElementById("upresult").innerHTML=returnValue;
            }  
            else  
            {  
                //alert("result is NULL !");  
            }  
        }   
        else   
        {  
            alert("page error!");  
        }  
    }  
}  

function upFileRet()   
{     
	//alert( "upfile");
    xhr = createXHR();  
    if(xhr)  
    {  
        xhr.onreadystatechange=upfilcallback;  
      	//alert("open()");
        xhr.open("GET", "result.cgi?cur_time=" + new Date().getTime());  
      
		//alert("send()");
        xhr.send(null);  
    }  
    else  
    {  
        alert("createXHR() error!");  
    }  
}
   
function firload()
{
	chtext();
	//upFileRet();
	setInterval(chtext,3000);
	//setInterval(upFileRet,1000);
}

//window.onload = firload;
