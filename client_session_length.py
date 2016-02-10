# client.py
import socket
import sys
import time
import threading
import random
from Queue import Queue

BUFFER_SIZE = 1024
PORT = 7000
HOSTNAME = 'localhost'
LOG_FILE_NAME = 'client data.csv'

def chatSession(hostname, port, message, sessionLenth, logFile):
    totalResponseTime = 0
    dataSent = 0

    try:
        # create a socket object
        clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # connect socket to the server
        clientSocket.connect((hostname, port))
    except socket.error, message:
        print 'Failed to create socket. Error code: ' + str(message[0]) + '-' + message[1]
        sys.exit()

    # insert random delay so we don't flood the server with text all at once
    #time.sleep(random.uniform(0.0, 1.0))
    sessionStartTime = time.time()
    timeElapsed = 0
    numOfMessages = 0

    while timeElapsed < sessionLenth:
        startTime = time.time()
        clientSocket.send(message)
        dataSent += len(message)
        # capture the echo from the server but no need to display it
        response = clientSocket.recv(BUFFER_SIZE)
        responseTime = time.time() - startTime
        totalResponseTime += responseTime
	numOfMessages += 1
	timeElapsed = time.time() - sessionStartTime

    averageResponseTime = totalResponseTime/numOfMessages
    logFile.put('%s,%s,%s\n' % (numOfMessages, dataSent, averageResponseTime))
    clientSocket.shutdown(socket.SHUT_RDWR) # close the connection
    clientSocket.close() # close the connection
    return

def writeToLogFile(queue, fileName):
    logFile = open(fileName, 'w')
    logFile.flush()

    while True:
        info = queue.get()
        if info == 'kill':
            break
        logFile.write(info)
        # write to file in real time
        logFile.flush()
    logFile.close()
    return

if __name__ == '__main__':
    if len(sys.argv) == 1: # length of sys.argv returns 1 if no command line argument was provided
        message = raw_input('message to send: ')
        sessionLenth = int(raw_input('how long should each session last (sec): '))
        numOfSessions = int(raw_input('number of clients to simulate: '))
    else:
        message = sys.argv[1]
        numOfMessages = int(sys.argv[2])

    logFileQueue = Queue()
    logFileThread = threading.Thread(target=writeToLogFile, args=(logFileQueue, LOG_FILE_NAME,))
    logFileThread.start()
    logFileQueue.put('requests,data sent,average response time\n')

    chatThreads = []

    for i in range (0, numOfSessions):
        chatThread = threading.Thread(target=chatSession, args=(HOSTNAME, PORT, message, sessionLenth, logFileQueue))
        chatThreads.append(chatThread)

    for thread in chatThreads:
        thread.start()
    for thread in chatThreads:
        thread.join()

    # close the log file
    logFileQueue.put('kill')
