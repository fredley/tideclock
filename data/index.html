<!DOCTYPE html>
<html lang="en">
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            body {
                color: #434343;
                font-family: "Helvetica Neue",Helvetica,Arial,sans-serif;
                font-size: 14px;
                line-height: 1.42857142857143;
                padding: 20px;
            }

            .container {
                margin: 0 auto;
                max-width: 400px;
            }

            form .field-group {
                box-sizing: border-box;
                clear: both;
                padding: 4px 0;
                position: relative;
                margin: 1px 0;
                width: 100%;
            }

            form .field-group > label {
                color: #757575;
                display: block;
                margin: 0 0 5px 0;
                padding: 5px 0 0;
                position: relative;
                word-wrap: break-word;
            }

            select,
            input[type=text] {
                background: #fff;
                border: 1px solid #d0d0d0;
                border-radius: 2px;
                box-sizing: border-box;
                color: #434343;
                font-family: inherit;
                font-size: inherit;
                height: 2.14285714em;
                line-height: 1.4285714285714;
                padding: 4px 5px;
                margin: 0;
                width: 100%;
            }

            select:focus,
            input[type=text]:focus {
                border-color: #4C669F;
                outline: 0;
            }

            .button-container {
                box-sizing: border-box;
                clear: both;
                margin: 10px 0 0;
                padding: 4px 0;
                position: relative;
                width: 100%;
            }

            button[type=submit] {
                box-sizing: border-box;
                background: #f5f5f5;
                border: 1px solid #bdbdbd;
                border-radius: 2px;
                color: #434343;
                cursor: pointer;
                display: inline-block;
                font-family: inherit;
                font-size: 14px;
                font-variant: normal;
                font-weight: 400;
                height: 2.14285714em;
                line-height: 1.42857143;
                margin: 0;
                padding: 4px 10px;
                text-decoration: none;
                vertical-align: baseline;
                white-space: nowrap;
            }

            form {
              opacity: 0;
              transition: opacity 0.1s ease-in-out;
            }
            form.visible {
              opacity: 1;
            }
            #done.visible,
            #loading.visible {
              display: block;
            }
            #done,
            #loading {
              font-size: 110%;
              color: #777;
              display: none;
            }
        </style>
        <title>Tide Clock</title>
        <script>
        document.addEventListener("DOMContentLoaded", function(event) {
          const httpRequest = new XMLHttpRequest()
          const list = document.getElementById("ssid")
          httpRequest.onreadystatechange = function(){
            if (httpRequest.readyState !== XMLHttpRequest.DONE) return
            if (httpRequest.status !== 200) return
            JSON.parse(httpRequest.responseText)
              .filter(d => d.ssid)
              .sort((a, b) => b.strength - a.strength)
              .forEach(d => {
                const option = document.createElement("option")
                const text = document.createTextNode(d.ssid)
                option.appendChild(text)
                list.appendChild(option)
              })
            document.getElementById("form").classList.add("visible")
            document.getElementById("loading").classList.remove("visible")
          };
          httpRequest.open('GET', '/scan', true);
          httpRequest.send();
        });

        function whenSubmit(event) {
          document.getElementById("form").classList.remove("visible")
          document.getElementById("done").classList.add("visible")
        }

        const form = document.getElementById('form');
        form.addEventListener('submit', whenSubmit);
        </script>
    </head>
    <body>
        <div class="container">
            <h1 style="text-align: center;">Tide Clock</h1>
            <p>Select your wifi network and enter the password to connect up your Tide Clock.</p>
            <p id="loading" class="visible">Scanning...</p>
            <p id="done" class="">Connecting...</p>
            <form method="post" action="/" id="form">
                <div class="field-group">
                    <label>Network</label>
                    <select name="ssid" type="text" id="ssid"></select>
                </div>
                <div class="field-group">
                    <label>Password</label>
                    <input name="password" type="text" size="64">
                </div>
                <div class="button-container">
                    <button type="submit">Connect</button>
                </div>
            </form>
        </div>
    </body>
</html>
