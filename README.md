# Dumper Documentation  

The RAT in desigen is a sophisticated malware that perform  
 malicious activities undetected by AV's from a remote Host.  
My goal was to create a Post Exploitation tool that will harvest  
user credentials off of windwos mechines running Windwos defender and  
diffrent Anti Virus Solutions by abusing the _Process Doppelgänging_ method.  

There are two main parts for the malware:
*   Firstly, malware staging and persistence.
*   Secondly, malware implementation and communication protocol.

Lets break down those two parts to get an in-depth look on  
how the entire procees would look from the post-infection point  
to the fully compermised state. 
&nbsp;  
  
## Staging & Persistence:
We want to achieve a multi-step staging proccess that will eventually  
load the malware implant into the infected computer in an unnoticed and stealthy way.  
Those are the componenets for securing this:  
1. Establishing a secure and encrypted connection with the _Drop Server_
    *  There will be an authentication routine, random key will be   
    exchanged  and associted with each infected computer.  
    Hence forming a unique IP & Key pair that will be able  
    to send authenticated request to the _Drop Server_.  
    
    &nbsp;

2.  Attaining the malware executable file
    *   We will request an encrypted and zipped version of the  
    malicious PE which will be called "Recovery.zip", then we  
    can unzip and decrypt it to obtain "update.exe" which  
    is the actual malware.

    &nbsp;

3. Achieving persistence 
    *   I chose to gain persistences by simply accessing the  
    _Windwos Registry_ and adding "update.exe" to the key _HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run_
