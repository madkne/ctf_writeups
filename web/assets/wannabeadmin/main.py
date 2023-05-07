#!/usr/bin/env python
from flask import Flask, session, render_template

from string import hexdigits
from random import choice


app = Flask(__name__)
secret = choice(hexdigits) + choice(hexdigits)
app.secret_key = secret * 8


@app.route("/", methods=["GET"])
def index():
    flag = "Only an Admin can see the FLAG!"
    if session and session["username"] == "admin":
        with open('flag.txt', 'r') as file:
            flag = file.read()

    return render_template("index.html", flag=flag)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=9001)
