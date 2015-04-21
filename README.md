UDP_FileTransfer
================
Features:

  use udp to transfer files 
  
  simple stop-wait protocol
  
  the receiver has a time-out timer, if time-out then retransmit ack 


Compile:
  gcc Makefile
  
Usage:

  ./main [s|m|l|q] [ip address]
  
  s--send file to the host that ip address specified.
  
  m--send msg to the host that ip address specified.
  
  l--list hosts in local network.
  
  q--quit.
  

