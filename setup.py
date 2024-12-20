from setuptools import setup, find_packages

# Чтение содержимого файла README.md
with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="tulpar",  # Имя вашего пакета
    version="0.1.2",  # Версия вашего пакета
    author="AnmiTali",  # Автор пакета
    author_email="nuros@anmitali.kz",  # Email автора
    description="The package manager for NurOS",  # Краткое описание
    long_description=long_description,  # Длинное описание из README.md
    long_description_content_type="text/markdown",  # Тип содержимого длинного описания
    url="https://github.com/nuros-linux/Tulpar",  # URL репозитория проекта
    packages=find_packages(),  # Автоматический поиск пакетов
    classifiers=[  # Классификаторы пакета
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: GPL License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',  # Минимальная версия Python
    install_requires=[  # Зависимости пакета
        # Например: 'requests>=2.24.0',
    ],
    entry_points={  # Настройка консольных скриптов
        'console_scripts': [
            'tulpar=tulpar.main:main',
        ],
    },
    include_package_data=True,  # Включение дополнительных данных, указанных в MANIFEST.in

    project_urls={  # Дополнительные URL проекта
        "Bug Tracker": "https://github.com/nuros-linux/Tulpar/issues",
        "Documentation": "https://github.com/nuros-linux/Tulpar/wiki",
        "Source Code": "https://github.com/nuros-linux/Tulpar",
    },
)