// ----------------------------------------------------------------------------------------------
// Copyright 2015 Mårten Rånge
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------------------------

open System
open System.Globalization
open System.IO
open System.Text
open System.Threading

open Microsoft.FSharp.Core.Printf

open MiniJson.JsonModule

[<EntryPoint>]
let main argv =
  Thread.CurrentThread.CurrentCulture   <- CultureInfo.InvariantCulture
  Thread.CurrentThread.CurrentUICulture <- CultureInfo.InvariantCulture

  try
    Environment.CurrentDirectory <- AppDomain.CurrentDomain.BaseDirectory

    let nonPrintableChars =
      [|
        for i in 0..31 ->
          match char i with
          | '\b'            -> @"\b"
          | '\f'            -> @"\f"
          | '\n'            -> @"\n"
          | '\r'            -> @"\r"
          | '\t'            -> @"\t"
          | ch              -> sprintf "\u%04X" i
      |]

    let rootPath    = Path.GetFullPath @"../../../test_cases/"
    let jsonPath    = Path.Combine (rootPath, "json")
    let resultPath  = Path.Combine (rootPath, "result")

    let testCases =
      Directory.GetFiles (jsonPath, @"*.json")
      |> Array.map (fun path ->
        let testCase    = Path.GetFileName path
        let jsonPath    = Path.GetFullPath path
        let resultPath  = Path.Combine (resultPath, testCase + ".result")
        testCase, jsonPath, resultPath
        )


    for testCase, jsonPath, resultPath in testCases do
      printfn "Processing: %s" testCase
      let sb                      = StringBuilder ()
      let inline str (s : string) = ignore <| sb.Append s
      let inline ch  (c : char)   = ignore <| sb.Append c
      let app (s : string)        =
        let e = s.Length - 1
        for i = 0 to e do
          match s.[i] with
          | '\"'            -> str @"\"""
          | '\\'            -> str @"\\"
          | '/'             -> str @"\/"
          | c when c < ' '  -> str nonPrintableChars.[int c]
          | c               -> ch c
        ignore <| sb.AppendLine ()
        true
      let appf f            = kprintf app f
      ignore <| appf "TestCase         : %s" testCase
      let v =
        { new IParseVisitor with
            member x.NullValue    ()        =           app   "NullValue        : null"
            member x.BoolValue    b         =           appf  "BoolValue        : %s" (if b then "true" else "false")
            member x.NumberValue  n         =           appf  "NumberValue      : %s" (n.ToString ("G", CultureInfo.InvariantCulture))
            member x.StringValue  s         =           appf  "StringValue      : %s" (s.ToString ())
            member x.ArrayBegin   ()        =           app   "Array            : begin"
            member x.ArrayEnd     ()        =           app   "Array            : end"
            member x.ObjectBegin  ()        =           app   "Object           : begin"
            member x.ObjectEnd    ()        =           app   "Object           : end"
            member x.MemberKey    mk        =           appf  "MemberKey        : %s" <| mk.ToString ()
            member x.ExpectedChar (pos, ch) = ignore <| appf  "ExpectedChar     : %d, %s" pos (ch.ToString ())
            member x.Expected     (pos, tk) = ignore <| appf  "ExpectedToken    : %d, %s" pos tk
            member x.Unexpected   (pos, tk) = ignore <| appf  "UnexpectedToken  : %d, %s" pos tk
        }

      let json        = File.ReadAllText jsonPath
      let mutable pos = 0
      let result      = tryParse v json &pos

      ignore <| appf "ParsePosition    : %d" pos
      ignore <| appf "ParseResult      : %s" (if result then "true" else "false")

      File.WriteAllText (resultPath, sb.ToString ())

    printfn "Done"
    0
  with
  | e ->
    printfn "Exception: %A" e.Message
    999

// ----------------------------------------------------------------------------------------------
