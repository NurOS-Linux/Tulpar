from platform import machine
CURRENT_ARCH=machine()
del machine
from colorama import Fore
from os import getuid

ICONS={'info':Fore.BLUE+'*','warn':Fore.YELLOW+'!','what':Fore.CYAN+'?','success':Fore.GREEN+'🗸','fail':Fore.RED+'𐄂'}
def log(type,text:str):print(f'{Fore.WHITE}[{ICONS[type]}{Fore.WHITE}] {Fore.RESET}{text}')
def ask(question:str):
    log('what',question)
    try:
        while True:
            answer=input(f'\r{Fore.WHITE}[{Fore.GREEN}y{Fore.WHITE}/{Fore.RED}n{Fore.WHITE}]{Fore.RESET}: \r').lower()
            if answer=='y':return True
            elif answer=='n':return False
    except:exit(0)